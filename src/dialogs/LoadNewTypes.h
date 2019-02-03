#ifndef LOADNEWTYPES_H
#define LOADNEWTYPES_H

#include <QDialog>

namespace Ui {
class LoadNewTypes;
}

class LoadNewTypes : public QDialog
{
    Q_OBJECT

public:
    explicit LoadNewTypes(QWidget *parent = nullptr);
    ~LoadNewTypes();

private:
    Ui::LoadNewTypes *ui;
};

#endif // LOADNEWTYPES_H
