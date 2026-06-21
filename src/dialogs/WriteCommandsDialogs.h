#ifndef WRITECOMMANDSDIALOGS_H
#define WRITECOMMANDSDIALOGS_H

#include "CutterCommon.h"

#include <QDialog>

#include <memory>

namespace Ui {
class Base64EnDecodedWriteDialog;
class IncrementDecrementDialog;
class DuplicateFromOffsetDialog;
}

/**
 * @brief A dialog for writing Base64 encoded or decoded data to the current binary
 */
class Base64EnDecodedWriteDialog : public QDialog
{
    Q_OBJECT
public:
    explicit Base64EnDecodedWriteDialog(QWidget *parent = nullptr);
    ~Base64EnDecodedWriteDialog();
    enum Mode : ut8 { Encode, Decode };
    Mode getMode() const;
    QByteArray getData() const;

private:
    std::unique_ptr<Ui::Base64EnDecodedWriteDialog> ui;
};

/**
 * @brief A dialog to increment or decrement binary values by a user-defined amount
 */
class IncrementDecrementDialog : public QDialog
{
    Q_OBJECT
public:
    explicit IncrementDecrementDialog(QWidget *parent = nullptr);
    ~IncrementDecrementDialog();
    enum Mode : ut8 { Increase, Decrease };
    Mode getMode() const;
    ut8 getNBytes() const;
    ut64 getValue() const;

private:
    std::unique_ptr<Ui::IncrementDecrementDialog> ui;
};

/**
 * @brief A dialog that duplicates a block of bytes from a source offset to the current location
 */
class DuplicateFromOffsetDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DuplicateFromOffsetDialog(QWidget *parent = nullptr);
    ~DuplicateFromOffsetDialog();
    RVA getOffset() const;
    size_t getNBytes() const;

private:
    std::unique_ptr<Ui::DuplicateFromOffsetDialog> ui;

private slots:
    void refresh();
};

#endif // WRITECOMMANDSDIALOGS_H
