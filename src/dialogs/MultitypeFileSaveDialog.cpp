#include "MultitypeFileSaveDialog.h"

#include <QMessageBox>

MultitypeFileSaveDialog::MultitypeFileSaveDialog(QWidget *parent, const QString &caption,
                                                 const QString &directory)
    : QFileDialog(parent, caption, directory)
{
    this->setAcceptMode(AcceptMode::AcceptSave);
    this->setFileMode(QFileDialog::AnyFile);

    connect(this, &QFileDialog::filterSelected, this, &MultitypeFileSaveDialog::onFilterSelected);
}

void MultitypeFileSaveDialog::setTypes(
        const QVector<MultitypeFileSaveDialog::TypeDescription> &types, bool useDetection)
{
    this->hasTypeDetection = useDetection;
    this->types.clear();
    this->types.reserve(types.size() + (useDetection ? 1 : 0));
    if (useDetection) {
        this->types.push_back(TypeDescription { tr("Detect type (*)"), "", QVariant() });
    }
    this->types.append(types);
    QStringList filters;
    for (auto &type : this->types) {
        filters.append(type.description);
    }
    setNameFilters(filters);
    onFilterSelected(this->types.first().description);
}

MultitypeFileSaveDialog::TypeDescription MultitypeFileSaveDialog::selectedType() const
{
    auto filterIt = findType(this->selectedNameFilter());
    if (filterIt == this->types.end()) {
        return {};
    }
    if (hasTypeDetection && filterIt == this->types.begin()) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
        const QFileInfo info(this->selectedFiles().constFirst());
#else
        const QFileInfo info(this->selectedFiles().first());
#endif
        QString currentSuffix = info.suffix();
        filterIt = std::find_if(types.begin(), types.end(),
                                [&currentSuffix](const TypeDescription &v) {
                                    return currentSuffix == v.extension;
                                });
        if (filterIt != types.end()) {
            return *filterIt;
        }
        return {};
    } else {
        return *filterIt;
    }
}

void MultitypeFileSaveDialog::done(int r)
{
    if (r == QDialog::Accepted) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
        const QFileInfo info(selectedFiles().constFirst());
#else
        const QFileInfo info(selectedFiles().first());
#endif
        auto selectedType = this->selectedType();
        if (selectedType.extension.isEmpty()) {
            QMessageBox::warning(this, tr("File save error"),
                                 tr("Unrecognized extension '%1'").arg(info.suffix()));
            return;
        }
    }
    QFileDialog::done(r);
}

void MultitypeFileSaveDialog::onFilterSelected(const QString &filter)
{
    auto it = findType(filter);
    if (it == types.end()) {
        return;
    }
    const bool detectionSelected = hasTypeDetection && it == types.begin();
    if (detectionSelected) {
        setDefaultSuffix(types[1].extension);
    } else {
        setDefaultSuffix(it->extension);
    }
    if (!this->selectedFiles().empty()) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
        const QString currentSelection = this->selectedFiles().constFirst();
#else
        const QString currentSelection = this->selectedFiles().first();
#endif
        const QFileInfo info(currentSelection);
        if (!detectionSelected) {
            const QString currentSuffix = info.suffix();
            if (currentSuffix != it->extension) {
                selectFile(info.dir().filePath(info.completeBaseName() + "." + it->extension));
            }
        }
    }
}

QVector<MultitypeFileSaveDialog::TypeDescription>::const_iterator
MultitypeFileSaveDialog::findType(const QString &description) const
{
    return std::find_if(types.begin(), types.end(), [&description](const TypeDescription &v) {
        return v.description == description;
    });
}
