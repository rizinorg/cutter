#ifndef SECTIONSDOCK_H
#define SECTIONSDOCK_H

#include "dockwidget.h"

class MainWindow;
class SectionsWidget;

namespace Ui
{
    class SectionsDock;
}

class SectionsDock : public DockWidget
{
    Q_OBJECT

public:
    explicit SectionsDock(MainWindow *main, QWidget *parent = 0);
    ~SectionsDock();

    void setup() override;

    void refresh() override;

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:

    void showSectionsContextMenu(const QPoint &pt);

    void on_actionVertical_triggered();

    void on_actionHorizontal_triggered();

private:
    Ui::SectionsDock *ui;
    MainWindow      *main;
    SectionsWidget  *sectionsWidget;
};

#endif // SECTIONSDOCK_H
