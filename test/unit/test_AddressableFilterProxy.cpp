/**
 * @file test_AddressableFilterProxy.cpp
 * @brief Address and name lookup through a sorted, filtered list proxy.
 */

#include "IsolatedSettings.h"

#include "common/AddressableItemModel.h"

#include <QStandardItemModel>
#include <QtTest/QtTest>

namespace {

constexpr int addressRole = Qt::UserRole;

class AddressListModel : public AddressableItemModel<QStandardItemModel>
{
public:
    void addItem(const QString &name, RVA address)
    {
        auto *item = new QStandardItem(name);
        item->setData(QVariant::fromValue<qulonglong>(address), addressRole);
        appendRow(item);
    }

    RVA address(const QModelIndex &index) const override
    {
        return index.isValid() ? data(index, addressRole).toULongLong() : RVA_INVALID;
    }

    QString name(const QModelIndex &index) const override
    {
        return index.isValid() ? data(index).toString() : QString();
    }
};

struct Item
{
    QString name;
    RVA address;
};

const QList<Item> items = {
    { "main", 0x1040 }, { "entry0", 0x1000 }, { "sym.helper", 0x1100 }, { "fcn.00001200", 0x1200 }
};

RVA addressOf(const QString &name)
{
    for (const Item &item : items) {
        if (item.name == name) {
            return item.address;
        }
    }
    return RVA_INVALID;
}

} // namespace

class TestAddressableFilterProxy : public QObject
{
    Q_OBJECT

private slots:
    void addressAndNameFollowSortedFilteredRows();
};

/**
 * @brief address() and name() of a proxy row belong to the source row shown there, after sorting
 *        and filtering.
 *
 * Every list dock seeks to, renames or comments the item through these, so a wrong mapping acts on
 * another item than the one the user clicked.
 */
void TestAddressableFilterProxy::addressAndNameFollowSortedFilteredRows()
{
    AddressListModel source;
    for (const Item &item : items) {
        source.addItem(item.name, item.address);
    }
    AddressableFilterProxyModel proxy(&source, nullptr);
    proxy.sort(0, Qt::DescendingOrder);
    proxy.setFilterFixedString("e");

    QCOMPARE(proxy.rowCount(), 2);
    for (int row = 0; row < proxy.rowCount(); ++row) {
        const QModelIndex index = proxy.index(row, 0);
        const QString shown = proxy.data(index).toString();
        QVERIFY(shown.contains('e'));
        QCOMPARE(proxy.name(index), shown);
        QCOMPARE(proxy.address(index), addressOf(shown));
    }
    QCOMPARE(proxy.address(QModelIndex()), RVA_INVALID);
}

CUTTER_UNIT_TEST_MAIN(TestAddressableFilterProxy)
#include "test_AddressableFilterProxy.moc"
