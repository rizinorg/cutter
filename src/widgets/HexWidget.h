#ifndef HEXWIDGET_H
#define HEXWIDGET_H

#include "Cutter.h"
#include "common/IOModesController.h"
#include "dialogs/HexdumpRangeDialog.h"

#include <QMenu>
#include <QScrollArea>
#include <QTimer>

#include <memory>

/**
 * @brief Tracks memory addresses while preventing 64-bit overflow
 */
struct BasicCursor
{
    uint64_t address;
    bool pastEnd;
    explicit BasicCursor(uint64_t pos) : address(pos), pastEnd(false) {}
    BasicCursor() : address(0), pastEnd(false) {}
    BasicCursor &operator+=(int64_t offset)
    {
        if (offset < 0 && uint64_t(-offset) > address) {
            address = 0;
            pastEnd = false;
        } else if (offset > 0 && uint64_t(offset) > (UINT64_MAX - address)) {
            address = UINT64_MAX;
            pastEnd = true;
        } else {
            address += uint64_t(offset);
            pastEnd = false;
        }
        return *this;
    }
    BasicCursor &operator+=(int offset)
    {
        *this += int64_t(offset);
        return *this;
    }

    bool moveChecked(int offset)
    {
        auto oldAddress = address;
        *this += offset;
        return address - oldAddress == uint64_t(offset);
    }

    BasicCursor &operator+=(uint64_t offset)
    {
        if (uint64_t(offset) > (UINT64_MAX - address)) {
            address = UINT64_MAX;
            pastEnd = true;
        } else {
            address += offset;
            pastEnd = false;
        }
        return *this;
    }
    bool operator<(const BasicCursor &r) const
    {
        return address < r.address || (pastEnd < r.pastEnd);
    }
};

/**
 * @brief Controls the cursor's visual blinking and screen location
 */
struct HexCursor
{
    HexCursor() : isVisible(false), onAsciiArea(false) {}

    bool isVisible;
    bool onAsciiArea;
    QTimer blinkTimer;
    QRectF screenPos;
    uint64_t address;
    QString cachedChar;
    QColor cachedColor;

    void blink() { isVisible = !isVisible; }
    void setBlinkPeriod(int msec) { blinkTimer.setInterval(msec / 2); }
    void startBlinking() { blinkTimer.start(); }
    void stopBlinking() { blinkTimer.stop(); }
};

/**
 * @brief A blueprint for how the widget reads and writes memory data.
 */
class AbstractData
{
public:
    virtual ~AbstractData() = default;
    virtual void fetch(uint64_t addr, int len) = 0;
    virtual bool copy(void *out, uint64_t adr, size_t len) = 0;
    virtual bool write(const uint8_t *in, uint64_t adr, size_t len) = 0;
    virtual uint64_t maxIndex() = 0;
    virtual uint64_t minIndex() = 0;
};

/**
 * @brief Handles memory operations for simple, local byte arrays
 */
class BufferData : public AbstractData
{
public:
    BufferData() { mBuffer.fill(0, 1); }

    explicit BufferData(const QByteArray &buffer)
    {
        if (buffer.isEmpty()) {
            mBuffer.fill(0, 1);
        } else {
            mBuffer = buffer;
        }
    }

    ~BufferData() override = default;

    void fetch(uint64_t, int) override {}

    bool copy(void *out, uint64_t addr, size_t len) override
    {
        if (addr < static_cast<uint64_t>(mBuffer.size())
            && (static_cast<uint64_t>(mBuffer.size()) - addr) < len) {
            memcpy(out, mBuffer.constData() + addr, len);
            return true;
        }
        return false;
    }

    bool write(const uint8_t *in, uint64_t addr, size_t len) override
    {
        if (addr < static_cast<uint64_t>(mBuffer.size())
            && (static_cast<uint64_t>(mBuffer.size()) - addr) < len) {
            memcpy(mBuffer.data() + addr, in, len);
            return true;
        }
        return false;
    }

    uint64_t maxIndex() override { return mBuffer.size() - 1; }

private:
    QByteArray mBuffer;
};

/**
 * @brief Manages live binary data using a fast block-based cache
 */
class MemoryData : public AbstractData
{
public:
    MemoryData() = default;
    ~MemoryData() override = default;
    static constexpr size_t blockSize = 4096;

    void fetch(uint64_t address, int length) override
    {
        // FIXME: reuse data if possible
        const uint64_t blockSize = 0x1000ULL;
        const uint64_t alignedAddr = address & ~(blockSize - 1);
        const int offset = address - alignedAddr;
        int len = (offset + length + (blockSize - 1)) & ~(blockSize - 1);
        mFirstBlockAddr = alignedAddr;
        mLastValidAddr = length ? alignedAddr + len - 1 : 0;
        if (mLastValidAddr < mFirstBlockAddr) {
            mLastValidAddr = -1;
            len = mLastValidAddr - mFirstBlockAddr + 1;
        }
        mBlocks.clear();
        uint64_t addr = alignedAddr;
        for (ut64 i = 0; i < len / blockSize; ++i, addr += blockSize) {
            mBlocks.append(Core()->ioRead(addr, blockSize));
        }
    }

    bool copy(void *out, uint64_t addr, size_t len) override
    {
        if (addr < mFirstBlockAddr
            || addr > mLastValidAddr
            /* do not merge with previous check to handle overflows */
            || (mLastValidAddr - addr + 1) < len || mBlocks.isEmpty()) {
            memset(out, 0xff, len);
            return false;
        }

        const int totalOffset = addr - mFirstBlockAddr;
        const int blockId = totalOffset / blockSize;
        const int blockOffset = totalOffset % blockSize;
        const size_t firstPart = blockSize - blockOffset;
        if (firstPart >= len) {
            memcpy(out, mBlocks.at(blockId).constData() + blockOffset, len);
        } else {
            memcpy(out, mBlocks.at(blockId).constData() + blockOffset, firstPart);
            memcpy(static_cast<char *>(out) + firstPart, mBlocks.at(blockId + 1).constData(),
                   len - firstPart);
        }
        return true;
    }

    void writeToCache(const uint8_t *in, uint64_t adr, size_t len)
    {
        if (adr < mFirstBlockAddr) {
            const uint64_t prefix = mFirstBlockAddr - adr;
            if (prefix <= len) {
                return;
            }
            in = in + prefix;
            adr += prefix;
            len -= prefix;
        }
        if (adr > mLastValidAddr) {
            return;
        }
        const int offset = (int)(adr - mFirstBlockAddr);
        int blockId = offset / blockSize;
        int blockOffset = offset % blockSize;
        while (len > 0 && blockId < mBlocks.size()) {
            size_t l = blockSize - blockOffset;
            l = std::min(l, len);
            memcpy(mBlocks[blockId].data() + blockOffset, in, l);
            len -= l;
            blockOffset = 0;
            adr += l;
            in += l;
            blockId += 1;
        }
    }

    bool write(const uint8_t *in, uint64_t adr, size_t len) override
    {
        RzCoreLocked core(Core());
        rz_core_write_at(core, adr, in, len);
        writeToCache(in, adr, len);
        emit Core()->instructionChanged(adr);
        return true;
    }

    uint64_t maxIndex() override { return std::numeric_limits<uint64_t>::max(); }

    uint64_t minIndex() override { return mFirstBlockAddr; }

private:
    QVector<QByteArray> mBlocks;
    uint64_t mFirstBlockAddr = 0;
    uint64_t mLastValidAddr = 0;
};

class HexSelection
{
public:
    HexSelection() : empty(true) { mStart = mEnd = 0; }

    inline void init(BasicCursor addr)
    {
        empty = true;
        mInit = addr;
    }

    void set(uint64_t start, uint64_t end)
    {
        empty = false;
        mInit = BasicCursor(start);
        mStart = start;
        mEnd = end;
    }

    void update(BasicCursor addr)
    {
        empty = false;
        if (mInit < addr) {
            mStart = mInit.address;
            mEnd = addr.address;
            if (!addr.pastEnd) {
                mEnd -= 1;
            }
        } else if (addr < mInit) {
            mStart = addr.address;
            mEnd = mInit.address;
            if (!mInit.pastEnd) {
                mEnd -= 1;
            }
        } else {
            mStart = mEnd = mInit.address;
            empty = true;
        }
    }

    bool intersects(uint64_t start, uint64_t end) const
    {
        return !empty && mEnd >= start && mStart <= end;
    }

    bool contains(uint64_t pos) const { return !empty && mStart <= pos && pos <= mEnd; }

    uint64_t size() const
    {
        uint64_t size = 0;
        if (!isEmpty()) {
            size = mEnd - mStart + 1;
        }
        return size;
    }

    inline bool isEmpty() const { return empty; }
    inline uint64_t start() const { return mStart; }
    inline uint64_t end() const { return mEnd; }

private:
    BasicCursor mInit;
    uint64_t mStart;
    uint64_t mEnd;
    bool empty;
};

class AddressRangeScrollBar;

/**
 * @brief Widget for rendering and editing of hex and ASCII memory data
 */
class HexWidget : public QScrollArea
{
    Q_OBJECT

public:
    explicit HexWidget(QWidget *parent = nullptr);
    ~HexWidget() override = default;

    void setMonospaceFont(const QFont &font);

    enum AddrWidth : ut8 { AddrWidth32 = 8, AddrWidth64 = 16 };
    enum ItemSize : ut8 {
        ItemSizeByte = 1,
        ItemSizeWord = 2,
        ItemSizeDword = 4,
        ItemSizeQword = 8
    };
    enum ItemFormat : ut8 {
        ItemFormatHex,
        ItemFormatOct,
        ItemFormatDec,
        ItemFormatSignedDec,
        ItemFormatFloat
    };
    enum class ColumnMode : ut8 { Fixed, PowerOf2 };
    enum class EditWordState : ut8 { Read, WriteNotStarted, WriteNotEdited, WriteEdited };
    enum class HexNavigationMode : ut8 { Words, WordChar, AnyChar };

    void setItemSize(int nbytes);
    void setItemFormat(ItemFormat format);
    void setItemEndianness(bool bigEndian);
    void setItemGroupSize(int size);
    /**
     * @brief Sets line size in bytes.
     * Changes column mode to fixed. Command can be rejected if current item format is bigger than
     * requested size.
     * @param bytes line size in bytes.
     */
    void setFixedLineSize(int bytes);
    void setColumnMode(ColumnMode mode);

    /**
     * @brief Select non empty inclusive range [start; end]
     * @param start
     * @param end
     */
    void selectRange(RVA start, RVA end);
    void clearSelection();

    struct Selection
    {
        bool empty;
        RVA startAddress;
        RVA endAddress;
    };
    Selection getSelection();
public slots:
    void seek(uint64_t address);
    void refresh();
    void updateColors();
signals:
    void selectionChanged(HexWidget::Selection selection);
    void positionChanged(RVA start);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    bool event(QEvent *event) override;

private slots:
    void onCursorBlinked();
    void onHexPairsModeEnabled(bool enable);
    void copy();
    void copyAddress();
    void onRangeDialogAccepted();
    void onActionAddCommentTriggered();
    void onActionDeleteCommentTriggered();
    void onActionAddFlagTriggered();
    void onActionAddMarkTriggered();
    void onActionDeleteMarkTriggered(const QString &name);
    void onActionEditMarkTriggered(const QString &name);

    // Write command slots
    void wWriteString();
    void wIncreaseDecrease();
    void wWriteBytes();
    void wWriteZeros();
    void wWrite64();
    void wWriteRandom();
    void wDuplFromOffset();
    void wWritePascalString();
    void wWriteWideString();
    void wWriteCString();

    void onKeyboardEditTriggered(bool enabled);
    void onKeyboardEditChanged(bool enabled);

private:
    void updateItemLength();
    void updateCounts();
    void drawHeader(QPainter &painter);
    void drawCursor(QPainter &painter, bool shadow = false);
    void drawAddrArea(QPainter &painter);
    void drawItemArea(QPainter &painter);
    void drawAsciiArea(QPainter &painter);
    void fillSelectionBackground(QPainter &painter, bool ascii = false);
    void fillMarks(QPainter &painter, bool ascii);
    void updateMetrics();
    void updateAreasPosition();
    void updateAreasHeight();
    enum class OverflowMove : ut8 { Clamp, Ignore };
    bool moveCursor(int offset, bool select = false,
                    OverflowMove overflowMove = OverflowMove::Clamp);
    void moveCursorKeepEditOffset(int byteOffset, bool select, OverflowMove overflowMove);
    void setCursorAddr(BasicCursor addr, bool select = false);
    void updateCursorMeta();
    void setCursorOnAscii(bool ascii);
    bool isItemDifferentAt(uint64_t address);
    QColor itemColor(uint8_t byte);
    QVariant readItem(int offset, QColor *color = nullptr);
    QString renderItem(int offset, QColor *color = nullptr);
    QChar renderAscii(int offset, QColor *color = nullptr);
    QString getFlagsAndComment(uint64_t address);
    /**
     * @brief Get the location on which operations such as Writing should apply.
     * @return Start of selection if multiple bytes are selected. Otherwise, the curren seek of the
     * widget.
     */
    RVA getLocationAddress();

    void fetchData();
    /**
     * @brief Convert mouse position to address.
     * @param point mouse position in widget
     * @param middle start next position from middle of symbol. Use middle=true for vertical cursor
     * position between symbols, middle=false for insert mode cursor and getting symbol under
     * cursor.
     * @return
     */
    BasicCursor screenPosToAddr(const QPoint &point, bool middle = false,
                                int *wordOffset = nullptr) const;
    BasicCursor asciiPosToAddr(const QPoint &point, bool middle = false) const;
    BasicCursor currentAreaPosToAddr(const QPoint &point, bool middle = false) const;
    BasicCursor mousePosToAddr(const QPoint &point, bool middle = false) const;
    /**
     * @brief Rectangle for single item in data area.
     * @param offset relative to first byte on screen
     * @return
     */
    QRectF itemRectangle(int offset);
    /**
     * @brief Rectangle for single item in ascii area.
     * @param offset relative to first byte on screen
     * @return
     */
    QRectF asciiRectangle(int offset);
    QVector<QPolygonF> rangePolygons(RVA start, RVA last, bool ascii);
    void updateWidth();

    inline qreal itemWidth() const { return itemCharLen * charWidth; }

    inline int itemGroupCharLen() const { return itemCharLen * itemGroupSize; }

    inline int columnExCharLen() const { return itemGroupCharLen() + columnSpacing; }

    inline int itemGroupByteLen() const { return itemByteLen * itemGroupSize; }

    inline qreal columnWidth() const { return itemGroupCharLen() * charWidth; }

    inline qreal columnExWidth() const { return columnExCharLen() * charWidth; }

    inline qreal columnSpacingWidth() const { return columnSpacing * charWidth; }

    inline int itemRowCharLen() const { return itemColumns * columnExCharLen() - columnSpacing; }

    inline int itemRowByteLen() const { return rowSizeBytes; }

    inline int bytesPerScreen() const { return itemRowByteLen() * visibleLines; }

    inline qreal itemRowWidth() const { return itemRowCharLen() * charWidth; }

    inline qreal asciiRowWidth() const { return itemRowByteLen() * charWidth; }

    inline qreal areaSpacingWidth() const { return areaSpacing * charWidth; }

    inline uint64_t lastVisibleAddr() const { return (startAddress - 1) + bytesPerScreen(); }

    const QRectF &currentArea() const { return cursorOnAscii ? asciiArea : itemArea; }
    bool isFixedWidth() const;

    bool canKeyboardEdit();
    bool flushCurrentlyEditedWord();
    bool finishEditingWord(bool force = true);
    void maybeFlushCharEdit();
    void cancelEditedWord();
    void startEditWord();
    bool validCharForEdit(QChar digit);
    void movePrevEditCharAny();
    void typeOverwriteModeChar(QChar c);
    HexNavigationMode defaultNavigationMode();
    void refreshWordEditState();
    bool parseWord(const QString &word, uint8_t *buf, size_t bufferSize) const;
    bool handleAsciiWrite(QKeyEvent *event);
    bool handleNumberWrite(QKeyEvent *event);

    void writeZeros(uint64_t address, uint64_t length);

    void hideWarningRect();
    void showWarningRect(QRectF rect);

    void updateViewport();
    void scrollLines(int lines, bool clampToScrollBarRange = false);
    /**
     * @brief Sets the given address as the first visible address of the view
     * No action is taken if the address is already at the top
     * @param address Target RVA to display at the top
     */
    void setStartAddress(RVA address);

    /**
     * @brief Updates cursor visibility and metadata
     * The cursor is only visible if it is within the visible address range and selection is empty
     */
    void updateCursorStatus();

    bool cursorEnabled;
    bool cursorOnAscii;
    HexCursor cursor;
    HexCursor shadowCursor;

    HexSelection selection;
    bool updatingSelection;

    QRectF addrArea;
    QRectF itemArea;
    QRectF asciiArea;

    int itemByteLen = 1;
    int itemGroupSize = 1; ///< Items per group (default: 1), 2 in case of hexpair mode
    int rowSizeBytes = 16; ///< Line size in bytes
    int itemColumns = 16; ///< Number of columns, single column consists of itemGroupSize items
    int itemCharLen = 2;
    int itemPrefixLen = 0;
    ColumnMode columnMode;

    ItemFormat itemFormat;

    bool itemBigEndian;
    QString itemPrefix;

    int visibleLines;
    uint64_t startAddress;
    qreal charWidth;
    qreal lineHeight;
    int addrCharLen;
    QFont monospaceFont;

    bool showHeader;
    bool showAscii;
    bool showExHex;
    bool showExAddr;

    QColor borderColor;
    QColor backgroundColor;
    QColor defColor;
    QColor addrColor;
    QColor diffColor;
    QColor b0x00Color;
    QColor b0x7fColor;
    QColor b0xffColor;
    QColor printableColor;
    QColor warningColor;

    HexdumpRangeDialog rangeDialog;

    /* Spacings in characters */
    const int columnSpacing = 1;
    const int areaSpacing = 2;

    const QString hexPrefix = QStringLiteral("0x");

    QMenu *rowSizeMenu;
    QAction *actionRowSizePowerOf2;
    QList<QAction *> actionsItemSize;
    QList<QAction *> actionsItemFormat;
    QAction *actionItemBigEndian;
    QAction *actionHexPairs;
    QAction *actionCopy;
    QAction *actionCopyAddress;
    QAction *actionComment;
    QAction *actionDeleteComment;
    QAction *actionAddFlag;
    QAction *actionAddMark;
    QAction *actionSelectRange;
    QAction *actionKeyboardEdit;
    QList<QAction *> actionsWriteString;
    QList<QAction *> actionsWriteOther;

    std::unique_ptr<AbstractData> oldData;
    std::unique_ptr<AbstractData> data;
    IOModesController ioModesController;

    int editWordPos = 0;
    QString editWord;
    EditWordState editWordState = EditWordState::Read;
    HexNavigationMode navigationMode = HexNavigationMode::Words;
    enum class EarlyEditFlush : ut8 {
        OnFinish,
        EditNibble,
        EditFixedWidthChar,
        /* AllFormats(not implemented) */
    };
    EarlyEditFlush earlyEditFlush = EarlyEditFlush::EditFixedWidthChar;

    bool warningRectVisible = false;
    QRectF warningRect;
    QTimer warningTimer;

    AddressRangeScrollBar *vScrollBar;
};

#endif // HEXWIDGET_H
