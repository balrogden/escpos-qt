#ifndef ESCPOSPRINTER_H
#define ESCPOSPRINTER_H

#include <QObject>
#define EscPosQt6_EXPORTS 1
#include <escposexports.h>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
class QTextCodec;
#else
#include <QStringConverter>
#endif

class QIODevice;

namespace EscPosQt {

class ESC_POS_QT_EXPORT EscPosPrinter : public QObject
{
    Q_OBJECT
public:
    explicit EscPosPrinter(QIODevice *device, QObject *parent = nullptr);
    explicit EscPosPrinter(QIODevice *device, const QByteArray &codecName, QObject *parent = nullptr);

    struct QRCode {
        enum Model {
            Model1 = 49,
            Model2,
            MicroQRCode,
        };
        enum ErrorCorrection {
            L = 48, // 7%
            M, // 15%
            Q, // 25%
            H, // 30%
        };
        QRCode(EscPosPrinter::QRCode::Model model, int moduleSize, EscPosPrinter::QRCode::ErrorCorrection erroCorrection, const QByteArray &_data);
        QByteArray data;
    };
    struct BarCode {
        enum Type {
          UPCA = 1, //only digits, len in (11,12)
          UPC_E, //only digits, len in (11,12)
          EAN13, //only digits, len in (11-13)
          EAN8, //only digits, len in (1-7)
          CODE39, //alpanumeric(latin) + (\x32,\x36,\x37,\x43,\x45,\x46,\x47), len in (1-12)
          ITF25, //only digits, len in (1-12)
          CODABAR, //alpanumeric(latin) + (\x36,\x43,\x45,\x46,\x47), len in (1-12)
          CODE93, //alpanumeric(latin) + (\x32,\x36,\x37,\x43,\x45,\x46,\x47), len in (1-12)
          CODE128A, //char in (0-127), len in (1-12)
        };
        BarCode(EscPosPrinter::BarCode::Type model, int width, int height, const QByteArray &_data):model(model),width(width),height(height),data(_data){};
        EscPosPrinter::BarCode::Type model;
        int width, height;
        QByteArray data;
    };
    struct PDF417BarCode {
        PDF417BarCode(int colCount, int rowCount, int width, int height, const QByteArray &_data)
          :colCount(colCount), rowCount(rowCount), width(width), height(height), data(_data){};
        int colCount, rowCount, width, height;
        QByteArray data;
    };

    enum PrintMode {
        PrintModeNone = 0x00, // 32char on mini, 48 on 80mm
        PrintModeFont2 = 0x01,
        PrintModeEmphasized = 0x08,
        PrintModeDoubleHeight = 0x10,
        PrintModeDoubleWidth = 0x20, // 16char on mini, 24 on 80mm
        PrintModeItalic = 0x40,
        PrintModeUnderline = 0x80,
    };
    Q_ENUM(PrintMode)
    Q_DECLARE_FLAGS(PrintModes, PrintMode)

    enum Justification {
        JustificationLeft = 0x30,
        JustificationCenter = 0x31,
        JustificationRight = 0x32,
    };
    Q_ENUM(Justification)

    enum HriPosition {
        HriNotPrinted = 0x00,
        HriNotAbove = 0x01,
        HriNotBelow = 0x02,
        HriNotAboveAndBelow = 0x03,
    };
    Q_ENUM(HriPosition)

    enum Encoding {
        EncodingPC437 = 0,
        EncodingKatakana = 1,
        EncodingPC850 = 2,// Qt supported
        EncodingPC860 = 3,
        EncodingPC866 = 17,// Qt supported
        EncodingPC852 = 18,
        EncodingISO8859_2 = 39,// Qt supported
        EncodingISO8859_15 = 40,// Qt supported
        EncodingWPC1250 = 45,// Qt supported
        EncodingWPC1251 = 46,// Qt supported
        EncodingWPC1253 = 47,// Qt supported
        EncodingWPC1254 = 48,// Qt supported
        EncodingWPC1255 = 49,// Qt supported
        EncodingWPC1256 = 50,// Qt supported
        EncodingWPC1257 = 51,// Qt supported
        EncodingWPC1258 = 52,// Qt supported
    };
    Q_ENUM(Encoding)

    struct _feed { int _lines; };
    inline static _feed feed(int __lines) { return { __lines }; }
private:
    int leftMargin = 0;
    int printingWidth = 0;
    PrintModes printModes;
    void transmitLeftMargin();
    void transmitPrintingWidth();
public:

    EscPosPrinter &operator<<(PrintModes i);
    EscPosPrinter &operator<<(PrintMode i);
    EscPosPrinter &operator>>(PrintMode i);
    EscPosPrinter &operator<<(Justification i);
    EscPosPrinter &operator<<(Encoding i);
    EscPosPrinter &operator<<(_feed lines);
    EscPosPrinter &operator<<(const char *s);
    EscPosPrinter &operator<<(const QByteArray &s);
    EscPosPrinter &operator<<(const QRCode &qr);
    EscPosPrinter &operator<<(const BarCode &bc);
    EscPosPrinter &operator<<(const PDF417BarCode &bc);
    EscPosPrinter &operator<<(const QImage &img); // img.width % 32 == 0
    /*!
     * The UTF-8 string will be encoded with QTextCodec
     * if one of the Qt supported encodings is selected.
     */
    EscPosPrinter &operator<<(const QString &text);
    EscPosPrinter &operator<<(QStringView text);
    EscPosPrinter &operator<<(void (*pf) ());

    static void init() {}
    static void eol() {}
    static void standardMode() {}
    static void pageMode() {}

    void write(const QByteArray &data);
    void write(const char *data, int size);

    EscPosPrinter &initialize();
    EscPosPrinter &encode(Encoding codec);
    EscPosPrinter &mode(PrintModes pm);
    EscPosPrinter &modeStandard();
    EscPosPrinter &modePage();
    EscPosPrinter &partialCut();
    EscPosPrinter &totalCut();
    EscPosPrinter &printAndFeedPaper(quint8 n = 1);
    EscPosPrinter &presentTicket(quint8 stepCount);
    EscPosPrinter &align(Justification i);
    EscPosPrinter &paperFeed(int lines = 1);
    EscPosPrinter &setLeftMargin(int value);
    EscPosPrinter &setPrintingWidth(int value);
    EscPosPrinter &setFontSize(int x, int y);

    enum PrinterStatus {PrinterStatusOnline = 0x01,
                       PrinterStatusCoverOpened = 0x02,
                       PrinterStatusPaperEnd  = 0x04,
                       PrinterStatusPaperNearEnd  = 0x08,
                       PrinterStatusPaperJam  = 0x10,
                       PrinterStatusOtherDeviceError = 0x20};
    Q_DECLARE_FLAGS(PrinterStatuses, PrinterStatus);
    PrinterStatuses getStatus();

//public Q_SLOTS:
    //void getStatus();

private:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QTextCodec *m_codec = nullptr;
#else
    QStringEncoder m_codec;
#endif
    QIODevice *m_device;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(EscPosQt::EscPosPrinter::PrintModes)

#endif // ESCPOSPRINTER_H
