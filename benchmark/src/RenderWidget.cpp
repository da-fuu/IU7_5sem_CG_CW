#include "RenderWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>
#include <QtMath>
#include <QMessageBox>
#include <iostream>

RenderWidget::RenderWidget(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true); // Включаем отслеживание мыши без нажатия кнопок

    renderer = new Renderer(800, 600);
    scene = new Scene();

    // Инициализация: добавляем первую шестеренку в центр
    float step = scene->getGridStep();
    scene->addGear(0, 0, step*0.5f, 12);

    // Таймер игрового цикла (~30 FPS)
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &RenderWidget::gameLoop);
    timer->start(33);

    // Таймер для бенчмарка (срабатывает раз в 1 секунду)
    benchmarkTimer = new QTimer(this);
    connect(benchmarkTimer, &QTimer::timeout, this, &RenderWidget::onBenchmarkStep);
}

RenderWidget::~RenderWidget() {
    if (csvFile.is_open()) {
        csvFile.close();
    }
    delete renderer;
    delete scene;
}

// --- BENCHMARK LOGIC ---

void RenderWidget::toggleBenchmark() {
    isBenchmarking = !isBenchmarking;

    if (isBenchmarking) {
        // 1. Отдаляем камеру для обзора
        camera.zoom(500.0f - camera.getPosition().length()); // Сброс к 500 (упрощенно)
        // Или правильнее через сеттер, если бы он был, но zoom() работает инкрементально.
        // Поэтому просто сбросим в конструкторе камеры или сделаем так:
        // camera.setRadius(500.0f); // Если добавите такой метод в Camera.h

        // Временный хак для зума (так как у нас только инкрементальный zoom)
        // Просто вызываем update() один раз с большим зумом в логике

        csvFile.open("benchmark_results.csv");
        if (csvFile.is_open()) {
            csvFile << "GearsCount;AvgFrameTime_ms\n";
        }

        accumFrameTimeNs = 0;
        framesCounted = 0;

        // Сброс индекса спирали (начинаем искать с 1-го кольца, т.к. 0-й занят)
        spiralRadius = 1;

        benchmarkTimer->start(1000);
        QMessageBox::information(this, "Benchmark", "Тест начат. Добавление по спирали в зацепление.");
    } else {
        benchmarkTimer->stop();
        if (csvFile.is_open()) {
            csvFile.close();
        }
        QMessageBox::information(this, "Benchmark", "Тест завершен.");
    }
}

// Генератор координат гексагональной спирали
void RenderWidget::getSpiralCoord(int n, int& q, int& r) {
    if (n == 0) { q = 0; r = 0; return; }

    // Находим радиус кольца
    int radius = 1;
    while (3 * radius * (radius + 1) < n) {
        radius++;
    }

    // Определяем позицию внутри кольца
    // (Алгоритм упрощенный перебор, для бенчмарка сойдет и "грубый" перебор в onBenchmarkStep)
}

void RenderWidget::onBenchmarkStep() {
    // 1. Запись статистики
    if (framesCounted > 0 && csvFile.is_open()) {
        double avgTimeMs = (double(accumFrameTimeNs) / framesCounted) / 1000000.0;
        csvFile << scene->gears.size() << ";" << avgTimeMs << std::endl;
        accumFrameTimeNs = 0;
        framesCounted = 0;
    }

    // 2. Добавление шестеренки В ЗАЦЕПЛЕНИЕ
    bool placed = false;
    int maxSearchRadius = 15; // Ищем в радиусе 15 клеток

    // Параметры "Малой" шестеренки
    float gridStep = scene->getGridStep();
    float rad = gridStep * 0.5f;
    int teeth = 12;

    // Перебор по спирали (радиусам) от центра
    for (int r_dist = 1; r_dist <= maxSearchRadius && !placed; ++r_dist) {
        // Проходим по кубическим координатам x+y+z=0 на расстоянии r_dist
        int x = -r_dist;
        int y = 0;
        int z = r_dist; // Начальная точка кольца

        // 6 направлений в кубических координатах
        // (1, -1, 0), (1, 0, -1), (0, 1, -1), (-1, 1, 0), (-1, 0, 1), (0, -1, 1)
        int dx[] = {1, 1, 0, -1, -1, 0};
        int dy[] = {-1, 0, 1, 1, 0, -1};
        int dz[] = {0, -1, -1, 0, 1, 1};

        for (int i = 0; i < 6 && !placed; ++i) {
            for (int j = 0; j < r_dist && !placed; ++j) {
                // Преобразование Кубические -> Осевые (q, r)
                // q = x, r = z
                int q = x;
                int r = z;

                // Проверяем статус:
                // 1 = Зацепление (Connected) - то, что нам нужно!
                // 0 = Коллизия
                // 2 = Изолирована (не подходит)
                int status = scene->checkPlacementStatus(q, r, rad);

                // Дополнительная проверка: ячейка должна быть пустой
                bool occupied = false;
                for(const auto& g : scene->gears) if(g.q == q && g.r == r) occupied = true;

                if (!occupied && status == 1) { // status == 1 значит есть контакт с кем-то
                    scene->addGear(q, r, rad, teeth);
                    placed = true;
                }

                // Шаг по кольцу
                x += dx[i];
                y += dy[i];
                z += dz[i];
            }
        }
    }

    if (!placed) {
        toggleBenchmark();
        QMessageBox::warning(this, "Benchmark", "Не удалось найти место для зацепления (поле заполнено или тупик).");
    }
}

// --- GAME LOOP & PAINT ---

void RenderWidget::gameLoop() {
    scene->update(0.033f);
    update(); // Инициирует paintEvent
}

void RenderWidget::paintEvent(QPaintEvent*) {
    // Начало замера времени кадра
    if (isBenchmarking) {
        frameTimer.restart();
    }

    // Обработка изменения размера окна
    if (renderer->width != width() || renderer->height != height()) {
        delete renderer;
        renderer = new Renderer(width(), height());
    }

    // 1. Обновление камеры и рендерера
    camera.setAspectRatio((float)width() / height());
    renderer->setCamera(camera);

    // 2. Очистка буфера (цвет фона - темно-серый)
    renderer->clear(qRgb(20, 20, 25));

    // 3. Отрисовка слоев сцены
    drawBackground();       // Пол и сетка
    drawSceneObjects();     // Тени, оси, шестеренки

    if (previewEnabled) {
        drawPreviewGhost(); // Призрак под курсором
    }

    drawOverlay();          // Текст поверх 3D

    // Конец замера времени кадра
    if (isBenchmarking) {
        accumFrameTimeNs += frameTimer.nsecsElapsed();
        framesCounted++;
    }
}

// --- DRAWING HELPERS ---

void RenderWidget::drawBackground() {
    // Рисуем пол (сплошной меш)
    QMatrix4x4 floorModel; // Identity
    QRgb floorColor = qRgb(180, 180, 180);

    for(const auto& tri : scene->floorMesh) {
         renderer->drawTriangle(tri, floorModel, false);
    }

    // Рисуем линии сетки поверх пола (используя QPainter)
    QPainter painter(&renderer->frameBuffer);
    painter.setRenderHint(QPainter::Antialiasing);
    QColor gridColor(50, 50, 50, 100);

    int r = scene->fieldRadius;
    for (int q = -r; q <= r; q++) {
        int r1 = std::max(-r, -q - r);
        int r2 = std::min(r, -q + r);
        for (int s = r1; s <= r2; s++) {
            QVector3D center = scene->hexToWorld(q, s);
            center.setY(scene->floorLevel + 0.05f); // Чуть выше пола, чтобы не мерцало
            drawHexagonOutline(painter, center, scene->hexSize, gridColor);
        }
    }
    painter.end();
}

void RenderWidget::drawSceneObjects() {
    // 1. Тени (Shadows)
    // Матрица планарной проекции на плоскость Y=0
    QMatrix4x4 shadowMat;
    if (std::abs(renderer->lightDir.y()) > 0.001f) {
        float kx = renderer->lightDir.x() / renderer->lightDir.y();
        float kz = renderer->lightDir.z() / renderer->lightDir.y();

        // x' = x - y*kx
        shadowMat.setRow(0, QVector4D(1, -kx, 0, 0));
        // y' = 0
        shadowMat.setRow(1, QVector4D(0, 0, 0, 0));
        // z' = z - y*kz
        shadowMat.setRow(2, QVector4D(0, -kz, 1, 0));
        // w' = 1
        shadowMat.setRow(3, QVector4D(0, 0, 0, 1));
    }

    for (auto& g : scene->gears) {
        QVector3D pos = scene->hexToWorld(g.q, g.r);

        QMatrix4x4 finalShadow;
        finalShadow.translate(0, 0.02f, 0); // Чуть приподнимаем тень над полом
        finalShadow = finalShadow * shadowMat; // Проецируем
        finalShadow.translate(pos.x(), scene->gearLevel, pos.z()); // Позиционируем объект
        finalShadow.rotate(qRadiansToDegrees(g.angle), 0, 1, 0);   // Вращаем

        for (const auto& tri : g.mesh) {
            renderer->drawTriangle(tri, finalShadow, true);
        }
    }

    // 2. Оси (Axles) - Статичные объекты
    for (auto& g : scene->gears) {
        QVector3D pos = scene->hexToWorld(g.q, g.r);
        QMatrix4x4 model;
        model.translate(pos.x(), scene->gearLevel, pos.z());

        for (const auto& tri : g.axleMesh) {
            renderer->drawTriangle(tri, model);
        }
    }

    // 3. Шестеренки (Gears) - Вращающиеся объекты
    for (auto& g : scene->gears) {
        QVector3D pos = scene->hexToWorld(g.q, g.r);
        QMatrix4x4 model;
        model.translate(pos.x(), scene->gearLevel, pos.z());
        model.rotate(qRadiansToDegrees(g.angle), 0, 1, 0);

        // Определяем цвет (Ведущая - Золотая, Ведомые - Голубые)
        // Заклиненные больше не красятся в красный (по ТЗ)
        QRgb baseColor = (g.isDriver) ? qRgb(255, 200, 50) : qRgb(100, 200, 255);

        for (const auto& tri : g.mesh) {
            // Создаем копию треугольника чтобы подменить цвет
            Triangle t = tri;
            t.color = baseColor;
            renderer->drawTriangle(t, model);
        }
    }
}

void RenderWidget::drawPreviewGhost() {
    // Рисуем призрак только если мышь над полем и не в режиме удаления
    if (!isHovering || currentMode == 3) return;

    // Рассчитываем параметры для выбранного режима
    float gridStep = scene->getGridStep();
    float rad = 0;
    int teeth = 0;
    float k = 12.0f / (gridStep * 0.5f); // Коэффициент зубьев

    if (currentMode == 0) { rad = gridStep * 0.5f; teeth = std::round(rad * k); }
    else if (currentMode == 1) { rad = gridStep * 1.5f; teeth = std::round(rad * k); }
    else if (currentMode == 2) { rad = gridStep * 2.5f; teeth = std::round(rad * k); }

    // Создаем временный объект Gear для получения меша
    Gear ghost(hoverQ, hoverR, rad, teeth, false);

    // Проверяем валидность размещения
    int status = scene->checkPlacementStatus(hoverQ, hoverR, rad);

    QRgb ghostColor;
    if (status == 0) ghostColor = qRgb(255, 0, 0);       // Коллизия (Красный)
    else if (status == 1) ghostColor = qRgb(0, 255, 0);  // Зацепление (Зеленый)
    else ghostColor = qRgb(150, 150, 150);               // Изолирована (Серый)

    QVector3D pos = scene->hexToWorld(hoverQ, hoverR);
    QMatrix4x4 model;
    model.translate(pos.x(), scene->gearLevel, pos.z());

    for (const auto& tri : ghost.mesh) {
        Triangle t = tri;
        t.color = ghostColor;
        // isGhost=true включает полупрозрачность и отключение записи в Z-буфер
        renderer->drawTriangle(t, model, false, true);
    }
}

void RenderWidget::drawOverlay() {
    QPainter widgetPainter(this);
    widgetPainter.drawImage(0, 0, renderer->frameBuffer);

    widgetPainter.setPen(Qt::white);
    QString infoText = "ЛКМ: Камера | ПКМ: Добавить/Удалить";

    if (isBenchmarking) {
        widgetPainter.setPen(Qt::yellow);
        infoText += QString(" | BENCHMARK: %1 objects").arg(scene->gears.size());
    }

    widgetPainter.drawText(10, 20, infoText);

    // Проверка на клин для вывода текста
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

// Отрисовка гексагона для сетки (в экранных координатах)
void RenderWidget::drawHexagonOutline(QPainter& painter, const QVector3D& center, float size, const QColor& color) {
    QPolygonF poly;
    bool allVisible = true;
    for (int i = 0; i < 6; i++) {
        float angle = i * M_PI / 3.0f + M_PI / 6.0f;
        QVector3D p3d = center + QVector3D(std::cos(angle) * size, 0, std::sin(angle) * size);
        QPointF p2d;
        if (renderer->project(p3d, p2d)) {
            poly << p2d;
        } else {
            allVisible = false;
        }
    }
    if (allVisible && !poly.isEmpty()) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(color, 1));
        painter.drawPolygon(poly);
    }
}

// --- INTERACTION ---

// Обновляет координаты hoverQ, hoverR на основе позиции мыши
void RenderWidget::updateHover(int mx, int my) {
    // Получаем луч из камеры
    QVector3D rayDir = camera.getRay(mx, my, width(), height());
    QVector3D camPos = camera.getPosition();

    // Пересечение луча с плоскостью Y = 0 (уровень пола для мыши)
    // Ray: P = O + t*D. Plane: y = 0.
    // O.y + t*D.y = 0  =>  t = -O.y / D.y
    if (std::abs(rayDir.y()) > 0.001f) {
        float t = -camPos.y() / rayDir.y();
        if (t > 0) {
            QVector3D hit = camPos + rayDir * t;

            // Обратное преобразование: Декартовы -> Гексагональные
            float qf = (hit.x() * std::sqrt(3.0f)/3.0f - hit.z()/3.0f) / scene->hexSize;
            float rf = (hit.z() * 2.0f/3.0f) / scene->hexSize;

            // Алгоритм округления кубических координат
            float x_grid = qf;
            float z_grid = rf;
            float y_grid = -x_grid - z_grid;

            int rx = round(x_grid);
            int ry = round(y_grid);
            int rz = round(z_grid);

            float x_diff = abs(rx - x_grid);
            float y_diff = abs(ry - y_grid);
            float z_diff = abs(rz - z_grid);

            if (x_diff > y_diff && x_diff > z_diff) rx = -ry - rz;
            else if (y_diff > z_diff) ry = -rx - rz;
            else rz = -rx - ry;

            hoverQ = rx;
            hoverR = rz;
            isHovering = true;
            return;
        }
    }
    isHovering = false;
}

void RenderWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::RightButton) {
        updateHover(e->pos().x(), e->pos().y());
        handleAction(); // Добавить или удалить
    } else if (e->button() == Qt::LeftButton) {
        isDragging = true;
        lastMousePos = e->pos();
    }
}

void RenderWidget::mouseMoveEvent(QMouseEvent* e) {
    if (isDragging) {
        int dx = e->pos().x() - lastMousePos.x();
        int dy = e->pos().y() - lastMousePos.y();

        // Вращение камеры
        camera.rotate(dy * 0.5f, dx * 0.5f);

        lastMousePos = e->pos();
    }

    // Обновляем позицию для предпросмотра
    updateHover(e->pos().x(), e->pos().y());
    update();
}

void RenderWidget::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) isDragging = false;
}

void RenderWidget::wheelEvent(QWheelEvent* e) {
    // Зум
    camera.zoom(e->angleDelta().y() * 0.1f);
    update();
}

void RenderWidget::handleAction() {
    if (!isHovering) return;
    int q = hoverQ;
    int r = hoverR;

    // Проверка границ поля
    int dist = (abs(q) + abs(r) + abs(q+r)) / 2;
    if (dist > scene->fieldRadius) return;

    if (currentMode == 3) {
        // Режим удаления
        scene->removeGear(q, r);
    } else {
        // Режим добавления
        float gridStep = scene->getGridStep();
        float rad = 0;
        int teeth = 0;
        float k = 12.0f / (gridStep * 0.5f);

        if (currentMode == 0) { rad = gridStep * 0.5f; teeth = std::round(rad * k); }
        else if (currentMode == 1) { rad = gridStep * 1.5f; teeth = std::round(rad * k); }
        else if (currentMode == 2) { rad = gridStep * 2.5f; teeth = std::round(rad * k); }

        if (scene->checkPlacementStatus(q, r, rad) == 0) {
            QMessageBox::warning(this, "Коллизия", "Невозможно разместить: пересечение с другой шестеренкой!");
        } else {
            scene->addGear(q, r, rad, teeth);
        }
    }
}
