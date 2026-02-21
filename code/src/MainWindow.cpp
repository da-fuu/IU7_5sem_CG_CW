#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "RenderWidget.h"
#include <QButtonGroup>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QButtonGroup* grp = new QButtonGroup(this);
    grp->addButton(ui->rbSmall, 0);
    grp->addButton(ui->rbMed, 1);
    grp->addButton(ui->rbLarge, 2);
    grp->addButton(ui->rbDel, 3);
    ui->rbSmall->setChecked(true);

    connect(grp, &QButtonGroup::idClicked, [this](int id){
        ui->renderWidget->currentMode = id;
    });

    connect(ui->cbPreview, &QCheckBox::toggled, [this](bool checked){
        ui->renderWidget->previewEnabled = checked;
        ui->renderWidget->update();
    });

    QButtonGroup* dirGrp = new QButtonGroup(this);
    dirGrp->addButton(ui->rbCW, 1);
    dirGrp->addButton(ui->rbCCW, 2);

    connect(dirGrp, &QButtonGroup::idClicked, [this](int id){
        ui->renderWidget->scene->motorDirection = (id == 2 ? 1 : -1);
    });

    connect(ui->speedSlider, &QSlider::valueChanged, [this](int val){
        ui->renderWidget->scene->motorSpeedVal = (float)val;
    });
}

MainWindow::~MainWindow() {
    delete ui;
}
