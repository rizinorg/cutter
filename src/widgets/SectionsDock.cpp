#include "SectionsDock.h"
#include "ui_SectionsDock.h"

#include "MainWindow.h"
#include "widgets/SectionsWidget.h"

#include <QMenu>
#include <QResizeEvent>


SectionsDock::SectionsDock(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::SectionsDock),
    main(main)
{
    ui->setupUi(this);

    this->sectionsWidget = new SectionsWidget(this->main);
    this->setWidget(this->sectionsWidget);
    this->sectionsWidget->setContentsMargins(0, 0, 0, 5);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showSectionsContextMenu(const QPoint &)));
}

SectionsDock::~SectionsDock() {}

void SectionsDock::showSectionsContextMenu(const QPoint &pt)
{
    // Set functions popup menu
    QMenu *menu = new QMenu(this);
    menu->clear();
    menu->addAction(ui->actionHorizontal);
    menu->addAction(ui->actionVertical);

    if (this->sectionsWidget->orientation() == 1) {
        ui->actionHorizontal->setChecked(true);
        ui->actionVertical->setChecked(false);
    } else {
        ui->actionVertical->setChecked(true);
        ui->actionHorizontal->setChecked(false);
    }

    this->setContextMenuPolicy(Qt::CustomContextMenu);
    menu->exec(this->mapToGlobal(pt));
    delete menu;
}

void SectionsDock::resizeEvent(QResizeEvent *event)
{
    if (main->responsive && isVisible()) {
        if (event->size().width() >= event->size().height()) {
            // Set horizontal view (list)
            this->on_actionHorizontal_triggered();
        } else {
            // Set vertical view (Tree)
            this->on_actionVertical_triggered();
        }
    }
    QWidget::resizeEvent(event);
}

void SectionsDock::on_actionVertical_triggered()
{
    this->sectionsWidget->setOrientation(Qt::Vertical);
}

void SectionsDock::on_actionHorizontal_triggered()
{
    this->sectionsWidget->setOrientation(Qt::Horizontal);
}
