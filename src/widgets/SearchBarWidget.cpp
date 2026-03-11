#include "SearchBarWidget.h"
#include "ui_SearchBarWidget.h"
#include "CutterSearchable.h"
#include "shortcuts/ShortcutManager.h"
#include "Configuration.h"

#include <QStyle>
#include <QStyleOption>
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QTimer>
#include <QBitArray>
#include <QSizeGrip>
#include <QMenu>
#include <QKeyEvent>

SearchBarSizeGrip::SearchBarSizeGrip(QWidget *parent) : QSizeGrip(parent)
{
    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
    this->setFixedWidth(5);
}

void SearchBarSizeGrip::moveEvent(QMoveEvent *moveEvent)
{
    QSizeGrip::moveEvent(moveEvent);
    setCursor(Qt::SizeHorCursor);
}

void SearchBarSizeGrip::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
}

SearchBarWidget::SearchBarWidget(QWidget *parent) : QWidget(parent), ui(new Ui::SearchBarWidget)
{
    ui->setupUi(this);

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 120));
    shadow->setOffset(0, 2);
    this->setGraphicsEffect(shadow);

    // buttons
    ui->menuButton->setAutoRaise(true);
    ui->menuButton->setPopupMode(QToolButton::InstantPopup);
    ui->menuButton->setStyleSheet("QToolButton { padding: 5px; }"
                                  "QToolButton::menu-indicator { width: 0px; image: none; }");
    ui->findNextButton->setAutoRaise(true);
    ui->findPrevButton->setAutoRaise(true);
    ui->findLastButton->setAutoRaise(true);
    ui->hideButton->setAutoRaise(true);

    ui->menuButton->setIcon(QIcon(":/img/icons/cog_light.svg"));
    ui->findNextButton->setIcon(QIcon(":/img/icons/arrow_right.svg"));
    ui->findPrevButton->setIcon(QIcon(":/img/icons/arrow_left.svg"));
    ui->findLastButton->setIcon(QIcon(":/img/icons/down_light.svg"));
    ui->hideButton->setIcon(QIcon(":/img/icons/delete_light.svg"));

    // removing this makes the buttons too close together in light theme
    const QString toolButtonPadding = "QToolButton { padding: 5px; }";
    ui->findNextButton->setStyleSheet(toolButtonPadding);
    ui->findPrevButton->setStyleSheet(toolButtonPadding);
    ui->findLastButton->setStyleSheet(toolButtonPadding);

    connect(ui->findNextButton, &QToolButton::clicked, this, &SearchBarWidget::findNextTriggered);
    connect(ui->findPrevButton, &QToolButton::clicked, this, &SearchBarWidget::findPrevTriggered);
    connect(ui->findLastButton, &QToolButton::clicked, this, &SearchBarWidget::findLastTriggered);
    connect(ui->hideButton, &QToolButton::clicked, this, &SearchBarWidget::hideSearchBar);

    // shortcuts
    QShortcut *findNextShortcut = Shortcuts()->makeQShortcut("Search.findNext", ui->searchEdit);
    QShortcut *findPrevShortcut = Shortcuts()->makeQShortcut("Search.findPrev", ui->searchEdit);
    QShortcut *findLastShortcut = Shortcuts()->makeQShortcut("Search.findLast", ui->searchEdit);
    QShortcut *hideShortcut = Shortcuts()->makeQShortcut("Search.hide", this);
    QShortcut *optionsShortcut = Shortcuts()->makeQShortcut("Search.options", this);
    hideShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(findNextShortcut, &QShortcut::activated, this, &SearchBarWidget::findNextTriggered);
    connect(findPrevShortcut, &QShortcut::activated, this, &SearchBarWidget::findPrevTriggered);
    connect(findLastShortcut, &QShortcut::activated, this, &SearchBarWidget::findLastTriggered);
    connect(hideShortcut, &QShortcut::activated, ui->hideButton, &QToolButton::click);
    connect(optionsShortcut, &QShortcut::activated, ui->menuButton, &QToolButton::click);

    // menu
    QMenu *optionsMenu = new QMenu(this);
    ui->menuButton->setMenu(optionsMenu);

    auto emitSearchChanged = [this] { emit searchChanged(text(), options()); };

    m_caseSensitiveAction = optionsMenu->addAction(tr("&Case Sensitive"));
    m_caseSensitiveAction->setCheckable(true);
    connect(m_caseSensitiveAction, &QAction::triggered, this, emitSearchChanged);

    m_wholeWordsAction = optionsMenu->addAction(tr("Match &Whole Words"));
    m_wholeWordsAction->setCheckable(true);
    connect(m_wholeWordsAction, &QAction::triggered, this, emitSearchChanged);

    m_regExpAction = optionsMenu->addAction(tr("Match &Regular Expression"));
    m_regExpAction->setCheckable(true);
    connect(m_regExpAction, &QAction::triggered, this, emitSearchChanged);

    m_highlightMatchesAction = optionsMenu->addAction(tr("&Highlight All Matches"));
    m_highlightMatchesAction->setCheckable(true);
    m_highlightMatchesAction->setChecked(true);
    connect(m_highlightMatchesAction, &QAction::triggered, this, emitSearchChanged);

    // debounce timer
    QTimer *debounceTimer = new QTimer(this);
    debounceTimer->setSingleShot(true);
    debounceTimer->setInterval(200);
    connect(debounceTimer, &QTimer::timeout, this,
            [this] { emit searchChanged(text(), options()); });
    connect(ui->searchEdit, &QLineEdit::textChanged, debounceTimer,
            static_cast<void (QTimer::*)()>(&QTimer::start));

    // size grip for resizing
    this->setWindowFlags(Qt::SubWindow);
    SearchBarSizeGrip *sg = new SearchBarSizeGrip(this);
    ui->horizontalLayout->insertWidget(0, sg);

    setFocusProxy(ui->searchEdit);
    this->setMinimumWidth(this->width() + 65); // account for spacing and margins

    // fixes the outline not showing around the widget where the label is present
    ui->searchLabel->setStyleSheet("QLabel { background: transparent; }");
}

SearchBarWidget::~SearchBarWidget() {}

void SearchBarWidget::setCurrentIndex(int index)
{
    m_index = index;
    updateLabel();
}

void SearchBarWidget::setTotalCount(int count)
{
    m_count = count;
    updateLabel();
}

void SearchBarWidget::setRange(int index, int count)
{
    m_index = index;
    m_count = count;
    updateLabel();
}

void SearchBarWidget::clear()
{
    m_index = 0;
    m_count = 0;
    updateLabel();
}

void SearchBarWidget::updateLabel()
{
    // indexes are 0-based
    int index = std::min(m_index + 1, m_count);
    ui->searchLabel->setText(tr("%1 of %2").arg(index).arg(m_count));
}

void SearchBarWidget::showSearchBar()
{
    this->show();
    this->raise();
    this->setFocus();
    emit showTriggered();
}

void SearchBarWidget::hideSearchBar()
{
    this->hide();
    emit hideTriggered();
}

void SearchBarWidget::selectText()
{
    ui->searchEdit->selectAll();
}

int SearchBarWidget::totalCount() const
{
    return m_count;
}

int SearchBarWidget::currentIndex() const
{
    return m_index;
}

QString SearchBarWidget::text() const
{
    return ui->searchEdit->text();
}

int SearchBarWidget::options() const
{
    int options = 0;
    options = m_caseSensitiveAction->isChecked() ? (options | CaseSensitive) : options;
    options = m_wholeWordsAction->isChecked() ? (options | WholeWords) : options;
    options = m_regExpAction->isChecked() ? (options | RegExp) : options;
    options = m_highlightMatchesAction->isChecked() ? (options | HighlightMatches) : options;
    return options;
}

void SearchBarWidget::paintEvent(QPaintEvent *event)
{
    // fill the background
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    QWidget::paintEvent(event);
}
