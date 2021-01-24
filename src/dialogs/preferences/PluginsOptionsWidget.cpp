
#include "PluginsOptionsWidget.h"

#include "PreferencesDialog.h"

#include "common/Helpers.h"
#include "common/Configuration.h"
#include "plugins/PluginManager.h"
#include "dialogs/RizinPluginsDialog.h"

#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QUrl>

PluginsOptionsWidget::PluginsOptionsWidget(PreferencesDialog *dialog) : QDialog(dialog)
{
    auto layout = new QVBoxLayout(this);
    setLayout(layout);

    auto dirLabel = new QLabel(this);
    dirLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    dirLabel->setOpenExternalLinks(true);
    layout->addWidget(dirLabel);
    auto pluginPath = Plugins()->getUserPluginsDirectory();
    dirLabel->setText(
            tr("Plugins are loaded from <a href=\"%1\">%2</a>")
                    .arg(QUrl::fromLocalFile(pluginPath).toString(), pluginPath.toHtmlEscaped()));

    auto treeWidget = new QTreeWidget(this);
    layout->addWidget(treeWidget);
    treeWidget->setRootIsDecorated(false);
    treeWidget->setHeaderLabels({ tr("Name"), tr("Description"), tr("Version"), tr("Author") });

    for (auto &plugin : Plugins()->getPlugins()) {
        auto item = new QTreeWidgetItem();
        item->setText(0, plugin->getName());
        item->setText(1, plugin->getDescription());
        item->setText(2, plugin->getVersion());
        item->setText(3, plugin->getAuthor());
        treeWidget->addTopLevelItem(item);
    }
    qhelpers::adjustColumns(treeWidget, 0);

    auto rzPluginsButton = new QPushButton(this);
    layout->addWidget(rzPluginsButton);
    rzPluginsButton->setText(tr("Show Rizin plugin information"));
    connect(rzPluginsButton, &QPushButton::clicked, this, [this]() {
        RizinPluginsDialog dialog(this);
        dialog.exec();
    });
}

PluginsOptionsWidget::~PluginsOptionsWidget() {}
