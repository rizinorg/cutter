#ifndef WRITECOMMANDSDIALOGS_H
#define WRITECOMMANDSDIALOGS_H

#include <QDialog>
#include "CutterCommon.h"

namespace Ui {
class Base64EnDecodedWriteDialog;
class IncrementDecrementDialog;
class DuplicateFromOffsetDialog;
}

class Base64EnDecodedWriteDialog : public QDialog
{
    Q_OBJECT
public:
    explicit Base64EnDecodedWriteDialog(QWidget *parent = nullptr);
    enum Mode { Encode, Decode };
    Mode getMode() const;
    QByteArray getData() const;

private:
    Ui::Base64EnDecodedWriteDialog* ui;
};

class IncrementDecrementDialog : public QDialog
{
    Q_OBJECT
public:
    explicit IncrementDecrementDialog(QWidget *parent = nullptr);
    enum Mode { Increase, Decrease };
    Mode getMode() const;
    uint8_t getNBytes() const;
    uint64_t getValue() const;

private:
    Ui::IncrementDecrementDialog* ui;
};

class DuplicateFromOffsetDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DuplicateFromOffsetDialog(QWidget *parent = nullptr);
    RVA getOffset() const;
    size_t getNBytes() const;

private:
    Ui::DuplicateFromOffsetDialog* ui;

private slots:
    void refresh();
};

#endif // WRITECOMMANDSDIALOGS_H
