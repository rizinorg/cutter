#ifndef DECOMPILERCONTEXTMENU_H
#define DECOMPILERCONTEXTMENU_H


#include "core/Cutter.h"
#include <QMenu>
#include <QKeySequence>

#include "common/IOModesController.h" 
/* I believe IOModesController
won't be required, copied from the disassemblycontextmenu for now. */

class DisassemblyContextMenu : public QMenu
{
    Q_OBJECT

public:
    DisassemblyContextMenu(QWidget *parent, MainWindow *mainWindow);
    ~DisassemblyContextMenu();

signals:
    void copy();

public slots:
    void setOffset(RVA offset);
    void setCanCopy(bool enabled);

    /**
     * @brief Sets the value of curHighlightedWord
     * @param text The current highlighted word
     */
    void setCurHighlightedWord(const QString &text);

private slots:
    void aboutToShowSlot();


    void on_actionCopy_triggered();

private:
    QKeySequence getCopySequence() const;


    RVA offset;
    bool canCopy;
    QString curHighlightedWord; // The current highlighted word
    MainWindow *mainWindow;

    QAction actionCopy;
    QAction *copySeparator;
   


    void initAction(QAction *action, QString name, const char *slot = nullptr);
    void initAction(QAction *action, QString name, const char *slot, QKeySequence keySequence);
    void initAction(QAction *action, QString name, const char *slot, QList<QKeySequence> keySequence);

    // struct ThingUsedHere {
    //     QString name;
    //     RVA offset;
    //     enum class Type {
    //         Var,
    //         Function,
    //         Flag,
    //         Address
    //     };
    //     Type type;
    // };
    // QVector<ThingUsedHere> getThingUsedHere(RVA offset);

    void updateTargetMenuActions(const QVector<ThingUsedHere> &targets);
};

#endif // DECOMPILERCONTEXTMENU_H