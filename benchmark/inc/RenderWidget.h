#ifndef RENDERWIDGET_H
#define RENDERWIDGET_H

#include <QWidget>
#include <QElapsedTimer> // Для замеров
#include <fstream>       // Для записи в файл
#include "Renderer.h"
#include "Scene.h"
#include "Camera.h"

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
    ~RenderWidget(); // Деструктор для закрытия файла

    // Метод запуска/остановки теста
    void toggleBenchmark();

private:
    QTimer* timer;

    // --- BENCHMARK DATA ---
    QTimer* benchmarkTimer;        // Таймер для добавления шестеренок (1 сек)
    QElapsedTimer frameTimer;      // Таймер для замера одного кадра
    bool isBenchmarking = false;
    long long accumFrameTimeNs = 0; // Накопленное время за секунду (наносекунды)
    int framesCounted = 0;          // Количество кадров за секунду
    std::ofstream csvFile;          // Файл для записи
    int spiralRadius = 0;           // Для поиска места под новую шестеренку
    // ----------------------

    int hoverQ = 0, hoverR = 0;
    bool isHovering = false;
    QPoint lastMousePos;
    bool isDragging = false;

    void gameLoop();
    void setupCameraMatrix();
    void drawBackground();
    void drawSceneObjects();
    void drawPreviewGhost();
    void drawOverlay();
    void drawHexagonOutline(QPainter& painter, const QVector3D& center, float size, const QColor& color);
    void getSpiralCoord(int n, int& q, int& r);
    void updateHover(int mx, int my);
    void handleAction();

    // Слот для добавления шестеренки и записи статистики
    void onBenchmarkStep();

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
};

#endif // RENDERWIDGET_H
