#include "DebugWidget.h"
#include "ui_DebugWidget.h"
#include "utils/Helpers.h"
#include "utils/JsonModel.h"
#include "utils/JsonTreeItem.h"
#include "utils/TempConfig.h"
#include "dialogs/VersionInfoDialog.h"


#include "MainWindow.h"

#include <QDebug>
#include <QJsonArray>
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QLayoutItem>
#include <QString>
#include <QMessageBox>
#include <QDialog>
#include <QTreeView>
#include <QTreeWidget>

DebugWidget::DebugWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::DebugWidget)
{
    ui->setupUi(this);

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(updateContents()));
}

DebugWidget::~DebugWidget() {}

void DebugWidget::updateContents()
{
    QJsonObject registerValues = Core()->getRegisterValues().object();
    QString strJson = Core()->getRegisterValues().toJson(QJsonDocument::Compact);

    int i = 0;
    QStringList registerNames = registerValues.keys();
    QGridLayout *registerLayout = new QGridLayout;
    int len = registerNames.size();
    int col = 0;
    for (const QString &key : registerNames) {
        QLabel *registerLabel = new QLabel(key);
        registerLabel->setStyleSheet("font-weight: bold");
        QLineEdit *registerEditValue = new QLineEdit;
        QString regValue = RAddressString(registerValues[key].toVariant().toULongLong());
        registerEditValue->setText(regValue);

        registerLayout->addWidget(registerLabel, i, col);
        registerLayout->addWidget(registerEditValue, i, col + 1);
        i++;
        if (i >= len/2) {
            i = 0;
            col = 2;
        }
    }
    ui->horizontalLayout_17->addLayout(registerLayout);
}
