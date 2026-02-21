#include "RenderWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>
#include <QtMath>
#include <QMessageBox>

RenderWidget::RenderWidget(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    renderer = new Renderer(800, 600);
    scene = new Scene();

    float step = scene->getGridStep();
    scene->addGear(0, 0, step*0.5f, 12);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &RenderWidget::gameLoop);
    timer->start(33);
}

void RenderWidget::gameLoop() {
    scene->update(0.033f);
    update();
}

void RenderWidget::paintEvent(QPaintEvent*) {
    if (renderer->width != width() || renderer->height != height()) {
        delete renderer;
        renderer = new Renderer(width(), height());
    }

    camera.setAspectRatio((float)width() / height());

    renderer->setCamera(camera);

    renderer->clear(qRgb(20, 20, 25));

    drawBackground();
    drawSceneObjects();
    if (previewEnabled) drawPreviewGhost();
    drawOverlay();
}

void RenderWidget::drawBackground() {
    QMatrix4x4 floorModel;
    for(const auto& tri : scene->floorMesh) {
         renderer->drawTriangle(tri, floorModel, false);
    }

    QPainter painter(&renderer->frameBuffer);
    painter.setRenderHint(QPainter::Antialiasing);
    QColor gridColor(50, 50, 50, 100);
    int r = scene->fieldRadius;
    for (int q = -r; q <= r; q++) {
        int r1 = std::max(-r, -q - r);
        int r2 = std::min(r, -q + r);
        for (int s = r1; s <= r2; s++) {
            QVector3D center = scene->hexToWorld(q, s);
            center.setY(scene->floorLevel + 0.05f);
            drawHexagonOutline(painter, center, scene->hexSize, gridColor);
        }
    }
    painter.end();
}

void RenderWidget::drawSceneObjects() {
    QMatrix4x4 shadowMat;
    if (std::abs(renderer->lightDir.y()) > 0.001f) {
        float kx = renderer->lightDir.x() / renderer->lightDir.y();
        float kz = renderer->lightDir.z() / renderer->lightDir.y();
        shadowMat.setRow(0, QVector4D(1, -kx, 0, 0));
        shadowMat.setRow(1, QVector4D(0, 0, 0, 0));
        shadowMat.setRow(2, QVector4D(0, -kz, 1, 0));
        shadowMat.setRow(3, QVector4D(0, 0, 0, 1));
    }

    for (auto& g : scene->gears) {
        QVector3D pos = scene->hexToWorld(g.q, g.r);
        QMatrix4x4 finalShadow;
        finalShadow.translate(0, 0.02f, 0);
        finalShadow = finalShadow * shadowMat;
        finalShadow.translate(pos.x(), scene->gearLevel, pos.z());
        finalShadow.rotate(qRadiansToDegrees(g.angle), 0, 1, 0);
        for (const auto& tri : g.mesh) renderer->drawTriangle(tri, finalShadow, true);
    }

    for (auto& g : scene->gears) {
        QVector3D pos = scene->hexToWorld(g.q, g.r);
        QMatrix4x4 model;
        model.translate(pos.x(), scene->gearLevel, pos.z());
        for (const auto& tri : g.axleMesh) renderer->drawTriangle(tri, model);
    }

    for (auto& g : scene->gears) {
        QVector3D pos = scene->hexToWorld(g.q, g.r);
        QMatrix4x4 model;
        model.translate(pos.x(), scene->gearLevel, pos.z());
        model.rotate(qRadiansToDegrees(g.angle), 0, 1, 0);

        QRgb baseColor = (g.isDriver) ? qRgb(255, 200, 50) : qRgb(100, 200, 255);
        for (const auto& tri : g.mesh) {
            Triangle t = tri; t.color = baseColor;
            renderer->drawTriangle(t, model);
        }
    }
}

void RenderWidget::drawPreviewGhost() {
    if (!isHovering || currentMode == 3) return;

    float gridStep = scene->getGridStep();
    float rad = 0;
    int teeth = 0;
    float k = 12.0f / (gridStep * 0.5f);

    if (currentMode == 0) rad = gridStep * 0.5f;
    else if (currentMode == 1) rad = gridStep * 1.5f;
    else if (currentMode == 2) rad = gridStep * 2.5f;
    //rad -= gridStep * 0.1f;
    teeth = std::round(rad * k);
    Gear ghost(hoverQ, hoverR, rad, teeth, false);
    int status = scene->checkPlacementStatus(hoverQ, hoverR, rad);
    QRgb ghostColor;
    if (status == 0) ghostColor = qRgb(255, 0, 0);
    else if (status == 1) ghostColor = qRgb(0, 255, 0);
    else ghostColor = qRgb(150, 150, 150);

    QVector3D pos = scene->hexToWorld(hoverQ, hoverR);
    QMatrix4x4 model;
    model.translate(pos.x(), scene->gearLevel - 0.5f, pos.z());
    for (const auto& tri : ghost.mesh) {
        Triangle t = tri; t.color = ghostColor;
        renderer->drawTriangle(t, model, false, true);
    }
}

void RenderWidget::drawOverlay() {
    QPainter widgetPainter(this);
    widgetPainter.drawImage(0, 0, renderer->frameBuffer);
    widgetPainter.setPen(Qt::white);
    widgetPainter.drawText(10, 20, "ЛКМ: Камера | ПКМ: Добавить/Удалить");

    bool jammed = false;
    for(auto& g : scene->gears) if(g.isJam) jammed = true;
    if(jammed) {
         widgetPainter.setPen(Qt::red);
         QFont f = widgetPainter.font();
         f.setBold(true);
         f.setPointSize(14);
         widgetPainter.setFont(f);
         widgetPainter.drawText(10, 50, "ЗАКЛИНИЛО!");
    }
}

void RenderWidget::drawHexagonOutline(QPainter& painter, const QVector3D& center, float size, const QColor& color) {
    QPolygonF poly;
    bool allVisible = true;
    for (int i = 0; i < 6; i++) {
        float angle = i * M_PI / 3.0f + M_PI / 6.0f;
        QVector3D p3d = center + QVector3D(std::cos(angle) * size, 0, std::sin(angle) * size);
        QPointF p2d;
        if (renderer->project(p3d, p2d)) poly << p2d;
        else allVisible = false;
    }
    if (allVisible && !poly.isEmpty()) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(color, 1));
        painter.drawPolygon(poly);
    }
}

void RenderWidget::updateHover(int mx, int my) {
    QVector3D rayDir = camera.getRay(mx, my, width(), height());
    QVector3D camPos = camera.getPosition();

    if (std::abs(rayDir.y()) > 0.001f) {
        float t = -camPos.y() / rayDir.y();
        if (t > 0) {
            QVector3D hit = camPos + rayDir * t;
            float qf = (hit.x() * std::sqrt(3.0f)/3.0f - hit.z()/3.0f) / scene->hexSize;
            float rf = (hit.z() * 2.0f/3.0f) / scene->hexSize;

            float x_grid = qf; float z_grid = rf; float y_grid = -x_grid - z_grid;
            int rx = round(x_grid); int ry = round(y_grid); int rz = round(z_grid);
            float x_diff = abs(rx - x_grid); float y_diff = abs(ry - y_grid); float z_diff = abs(rz - z_grid);
            if (x_diff > y_diff && x_diff > z_diff) rx = -ry - rz;
            else if (y_diff > z_diff) ry = -rx - rz;
            else rz = -rx - ry;

            hoverQ = rx; hoverR = rz;
            isHovering = true;
            return;
        }
    }
    isHovering = false;
}

void RenderWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::RightButton) {
        updateHover(e->pos().x(), e->pos().y());
        handleAction();
    } else if (e->button() == Qt::LeftButton) {
        isDragging = true;
        lastMousePos = e->pos();
    }
}

void RenderWidget::mouseMoveEvent(QMouseEvent* e) {
    if (isDragging) {
        int dx = e->pos().x() - lastMousePos.x();
        int dy = e->pos().y() - lastMousePos.y();

        camera.rotate(dy * 0.5f, dx * 0.5f);

        lastMousePos = e->pos();
    }
    updateHover(e->pos().x(), e->pos().y());
    update();
}

void RenderWidget::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) isDragging = false;
}

void RenderWidget::wheelEvent(QWheelEvent* e) {
    camera.zoom(e->angleDelta().y() * 0.1f);
    update();
}

void RenderWidget::handleAction() {
    if (!isHovering) return;
    int q = hoverQ;
    int r = hoverR;

    int dist = (abs(q) + abs(r) + abs(q+r)) / 2;
    if (dist > scene->fieldRadius) return;

    if (currentMode == 3) {
        scene->removeGear(q, r);
    } else {
        float gridStep = scene->getGridStep();
        float rad = 0;
        int teeth = 0;
        float k = 12.0f / (gridStep * 0.5f);

        if (currentMode == 0) { rad = gridStep * 0.5f; teeth = std::round(rad * k); }
        else if (currentMode == 1) { rad = gridStep * 1.5f; teeth = std::round(rad * k); }
        else if (currentMode == 2) { rad = gridStep * 2.5f; teeth = std::round(rad * k); }

        if (scene->checkPlacementStatus(q, r, rad) == 0) {
            QMessageBox::warning(this, "Коллизия", "Невозможно разместить: пересечение!");
        } else {
            scene->addGear(q, r, rad, teeth);
        }
    }
}
