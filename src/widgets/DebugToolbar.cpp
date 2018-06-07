#include "DebugToolbar.h"
#include "MainWindow.h"

#include <QAction>
#include <QPainter>

static QIcon getIconFor(const QString &str)
{
    int w = 64;
    int h = 64;

    QPixmap pixmap(w, h);
    pixmap.fill(Qt::transparent);

    QPainter pixPaint(&pixmap);
    pixPaint.setPen(Qt::NoPen);
    pixPaint.setRenderHint(QPainter::Antialiasing);
    pixPaint.setBrush(QBrush(QColor(52, 152, 219)));
    pixPaint.drawEllipse(1, 1, w - 2, h - 2);
    pixPaint.setPen(Qt::white);
    pixPaint.setFont(QFont("Verdana", 24, 1));
    pixPaint.drawText(0, 0, w, h - 2, Qt::AlignCenter, QString(str).toUpper().mid(0, 2));
    return QIcon(pixmap);
}

DebugToolbar::DebugToolbar(MainWindow *main, QWidget *parent) :
    QToolBar(parent),
    main(main)
{
    setObjectName("debugToolbar");
    QIcon iconPlaceholder = getIconFor("Start");
    QAction *actionStart = new QAction(getIconFor("Start"), tr("Start"), parent);
    QAction *actionContinue = new QAction(getIconFor("Continue"), tr("Continue"), parent);
    QAction *actionContinueUntilMain = new QAction(getIconFor("CU"), tr("Continue until main"), parent);
    QAction *actionStep = new QAction(getIconFor("Step"), tr("Step"), parent);
    QAction *actionStepOver = new QAction(getIconFor("SO"), tr("Step over"), parent);
    addAction(actionStart);
    addAction(actionContinue);
    addAction(actionContinueUntilMain);
    addAction(actionStep);
    addAction(actionStepOver);

    connect(actionStep,              &QAction::triggered, Core(), &CutterCore::stepDebug);
    connect(actionStart,             &QAction::triggered, Core(), &CutterCore::startDebug);
    connect(actionStepOver,          &QAction::triggered, Core(), &CutterCore::stepOverDebug);
    connect(actionContinue,          &QAction::triggered, Core(), &CutterCore::continueDebug);
    connect(actionContinueUntilMain, &QAction::triggered, this,   &DebugToolbar::continueUntilMain);
}

void DebugToolbar::continueUntilMain()
{
    Core()->continueUntilDebug(tr("main"));
}