#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "RenderWidget.h"
#include <QButtonGroup>
#include <QPushButton> // !

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Группа размеров
    QButtonGroup* grp = new QButtonGroup(this);
    grp->addButton(ui->rbSmall, 0);
    grp->addButton(ui->rbMed, 1);
    grp->addButton(ui->rbLarge, 2);
    grp->addButton(ui->rbDel, 3);

    connect(grp, &QButtonGroup::idClicked, [this](int id){
        ui->renderWidget->currentMode = id;
    });

    // Предпросмотр
    connect(ui->cbPreview, &QCheckBox::toggled, [this](bool checked){
        ui->renderWidget->previewEnabled = checked;
        ui->renderWidget->update();
    });

    // Мотор
    QButtonGroup* dirGrp = new QButtonGroup(this);
    dirGrp->addButton(ui->rbCW, 1);
    dirGrp->addButton(ui->rbCCW, 2);

    connect(dirGrp, &QButtonGroup::idClicked, [this](int id){
        ui->renderWidget->scene->motorDirection = (id == 2 ? 1 : -1);
    });

    connect(ui->speedSlider, &QSlider::valueChanged, [this](int val){
        ui->renderWidget->scene->motorSpeedVal = (float)val;
    });

    // --- НОВАЯ КНОПКА: ТЕСТ ПРОИЗВОДИТЕЛЬНОСТИ ---
    // Добавляем её программно в конец панели управления
    QPushButton* btnBench = new QPushButton("Тест производительности", this);
    btnBench->setCheckable(true); // Кнопка с фиксацией

    // Добавляем в layout (он называется verticalLayout внутри controlPanel в .ui файле)
    // Чтобы добавить, нам нужен доступ к layout. В сгенерированном ui он доступен.
    ui->verticalLayout->addWidget(btnBench);

    connect(btnBench, &QPushButton::toggled, [this](bool checked){
        ui->renderWidget->toggleBenchmark();
    });
}

MainWindow::~MainWindow() {
    delete ui;
}
