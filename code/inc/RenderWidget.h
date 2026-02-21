#ifndef RENDERWIDGET_H
#define RENDERWIDGET_H

#include <QWidget>
#include "Renderer.h"
#include "Scene.h"
#include "Camera.h" // !

class QTimer;

class RenderWidget : public QWidget {
    Q_OBJECT
public:
    Renderer* renderer;
    Scene* scene;
    Camera camera;

    int currentMode = 0;
    bool previewEnabled = false;

    RenderWidget(QWidget* parent = nullptr);

private:
    QTimer* timer;

    // Mouse state
    int hoverQ = 0, hoverR = 0;
    bool isHovering = false;
    QPoint lastMousePos;
    bool isDragging = false;

    void gameLoop();
    void drawBackground();
    void drawSceneObjects();
    void drawPreviewGhost();
    void drawOverlay();
    void drawHexagonOutline(QPainter& painter, const QVector3D& center, float size, const QColor& color);

    void updateHover(int mx, int my);
    void handleAction();

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
};

#endif // RENDERWIDGET_H
