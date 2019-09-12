#ifndef MULTITYPEFILESAVEDIALOG_H
#define MULTITYPEFILESAVEDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QVariant>

class MultitypeFileSaveDialog : public QFileDialog
{
    Q_OBJECT

public:
    struct TypeDescription {
        QString description;
        QString extension;
        QVariant data;
    };

    explicit MultitypeFileSaveDialog(QWidget *parent = nullptr,
                                     const QString &caption = QString(),
                                     const QString &directory = QString());

    void setTypes(const QVector<TypeDescription> types, bool useDetection = true);
    TypeDescription selectedType() const;
protected:
    void done(int r) override;
private:
    void onFilterSelected(const QString &filter);
    QVector<TypeDescription>::const_iterator findType(const QString &description) const;

    QVector<TypeDescription> types;
    bool hasTypeDetection;
};

#endif // MULTITYPEFILESAVEDIALOG_H
