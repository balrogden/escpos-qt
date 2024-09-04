#include "escposprinter.h"

#include <QIODevice>
#include <QDataStream>
#include <QImage>

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(EPP, "esc_pos")
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QTextCodec>
#endif
static const char ESC = 0x1B;
static const char GS = 0x1D;

using namespace EscPosQt;

EscPosPrinter::EscPosPrinter(QIODevice *device, QObject *parent) : QObject(parent)
  , m_device(device)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    m_codec = QTextCodec::codecForLocale();
#else
    m_codec = QStringEncoder();
#endif
    /*connect(m_device, &QIODevice::readyRead, this, [=] {
        const QByteArray data = m_device->readAll();
        qCDebug(EPP) << "GOT" << data << data.toHex();
    });*/
}

EscPosPrinter::EscPosPrinter(QIODevice *device, const QByteArray &codecName, QObject *parent) : QObject(parent)
  , m_device(device)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    m_codec = QTextCodec::codecForName(codecName);
#else
    m_codec = QStringEncoder(codecName);
#endif
    /*connect(m_device, &QIODevice::readyRead, this, [=] {
        const QByteArray data = m_device->readAll();
        qCDebug(EPP) << "GOT" << data << data.toHex();
    });*/
}

EscPosPrinter &EscPosPrinter::setFontSize(int x, int y)
{
  char str[] = { GS, '!',char( (((x<=0)?0:(x>=8)?7:(x-1))<<4) | ((y<=0)?0:(y>=8)?7:(y-1)) ) };
  qCDebug(EPP) << "setFontSize " << QByteArray(str, sizeof(str)) << QByteArray(str, sizeof(str)).toHex();
  write(str, sizeof(str));
  return *this;
}

EscPosPrinter &EscPosPrinter::operator<<(PrintMode i)
{
  char str[] = { ESC, 0, '\x01' };
  printModes.setFlag(i);
  switch(i)
  {
  case PrintModeNone:
    return *this;
  case PrintModeDoubleHeight:
  case PrintModeDoubleWidth:
    str[0] = GS;
    str[1] = '!';
    str[2] = (printModes.testFlag(PrintModeDoubleWidth)?0x00:0x10)|(printModes.testFlag(PrintModeDoubleHeight)?0x00:0x01);
    break;
  case PrintModeFont2:
    str[1] = '\x4D';
    str[2] = 49;
    break;
  case PrintModeEmphasized:
    str[1] = '\x45';
    break;
  case PrintModeItalic:
    str[1] = '4';
    break;
  case PrintModeUnderline:
    str[1] = '-';
    break;
  }
  qCDebug(EPP) << "+printMode " << QByteArray(str, sizeof(str)) << QByteArray(str, sizeof(str)).toHex();
  write(str, sizeof(str) );
  return *this;
}
EscPosPrinter &EscPosPrinter::operator>>(PrintMode i)
{
  char str[] = { ESC, 0, '\x00' };
  printModes.setFlag(i, false);
  switch(i)
  {
  case PrintModeNone:
    return *this;
  case PrintModeDoubleHeight:
  case PrintModeDoubleWidth:
    str[0] = GS;
    str[1] = '!';
    str[2] = (printModes.testFlag(PrintModeDoubleWidth)?0x00:0x10)|(printModes.testFlag(PrintModeDoubleHeight)?0x00:0x01);
    break;
  case PrintModeFont2:
    str[1] = '\x4D';
    str[2] = 48;
    break;
  case PrintModeEmphasized:
    str[1] = '\x45';
    break;
  case PrintModeItalic:
    str[1] = '4';
    break;
  case PrintModeUnderline:
    str[1] = '-';
    break;
  }
  qCDebug(EPP) << "-printMode " << QByteArray(str, sizeof(str)) << QByteArray(str, sizeof(str)).toHex();
  write(str, sizeof(str) );
  return *this;
}

EscPosPrinter &EscPosPrinter::operator<<(PrintModes i)
{
    return mode(i);
}

EscPosPrinter &EscPosPrinter::operator<<(EscPosPrinter::Justification i)
{
    return align(i);
}

EscPosPrinter &EscPosPrinter::operator<<(EscPosPrinter::Encoding i)
{
    return encode(i);
}

EscPosPrinter &EscPosPrinter::operator<<(EscPosPrinter::_feed lines)
{
    return paperFeed(lines._lines);
}

EscPosPrinter &EscPosPrinter::operator<<(const char *s)
{
    qCDebug(EPP) << "char *s" << QByteArray(s);
    write(s, int(strlen(s)));
    return *this;
}

EscPosPrinter &EscPosPrinter::operator<<(const QByteArray &s)
{
    write(s);
    return *this;
}

EscPosPrinter &EscPosPrinter::operator<<(const EscPosPrinter::QRCode &qr)
{
    write(qr.data);
    return *this;
}

EscPosPrinter &EscPosPrinter::operator<<(const BarCode &bc)
{
  const char str[] = { GS, 'w', char(bc.width>255?255:bc.width<1?1:bc.width)
                       , GS, 'h', char(bc.height>255?255:bc.height<12?12:bc.height)
                       , GS, '\x6b', char(bc.model) };
  const char strEnd[] = { 0 };
  write(str, sizeof(str));
  write(bc.data);
  write(strEnd, sizeof(strEnd));
  return *this;
};

EscPosPrinter &EscPosPrinter::operator<<(const PDF417BarCode &bc)
{
  const char str[] = { GS, '(', 'k', 0x03, 0x00, 0x30, 0x41, char(bc.colCount>255?255:bc.colCount<1?1:bc.colCount)
                     , GS, '(', 'k', 0x03, 0x00, 0x30, 0x42, char(bc.rowCount>255?255:bc.rowCount<1?1:bc.rowCount)
                     , GS, '(', 'k', 0x03, 0x00, 0x30, 0x43, char(bc.width>255?255:bc.width<1?1:bc.width)
                     , GS, '(', 'k', 0x03, 0x00, 0x30, 0x44, char(bc.height>255?255:bc.height<12?12:bc.height)
                     , GS, '(', 'k', 0x03, 0x00, 0x30, 0x45, 0x30, 0x02
                     , GS, '(', 'k', char((bc.data.length()+3)&0xff), char(((bc.data.length()+3)>>8)&0xff), 0x30, 0x50, 0x30 };
  const char strEnd[] = { GS, '(', 'k', 0x03, 0x00, 0x30, 0x51, 0x30 };
  write(str, sizeof(str));
  write(bc.data);
  write(strEnd, sizeof(strEnd));
  return *this;
};

EscPosPrinter &EscPosPrinter::operator<<(const QString &text)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    qCDebug(EPP) << "string" << text << text.toLatin1() << m_codec->fromUnicode(text);
    if (m_codec) {
        write(m_codec->fromUnicode(text));
    } else {
        write(text.toLatin1());
    }
#else
    qCDebug(EPP) << "string" << text << text.toLatin1() << m_codec.encode(text);
    QByteArray barr = m_codec.encode(text);
    write( barr );
#endif
    return *this;
}

EscPosPrinter &EscPosPrinter::operator<<(QStringView text)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    qCDebug(EPP) << "string" << text << text.toLatin1() << m_codec->fromUnicode(text);
    if (m_codec) {
        write(m_codec->fromUnicode(text));
    } else {
        write(text.toLatin1());
    }
#else
    qCDebug(EPP) << "string" << text << text.toLatin1() << m_codec.encode(text);
    write(m_codec.encode(text));
#endif
    return *this;
}

EscPosPrinter &EscPosPrinter::operator<<(void (*pf)())
{
    if (pf == EscPosPrinter::eol) {
        write("\n", 1);
    } else if (pf == EscPosPrinter::init) {
        return initialize();
    } else if (pf == EscPosPrinter::standardMode) {
        return modeStandard();
    } else if (pf == EscPosPrinter::pageMode) {
        return modePage();
    }
    return *this;
}

EscPosPrinter &EscPosPrinter::operator<<(const QImage &img)
{
  if(img.colorCount()==2)
  {
    const char str[] = { GS, '\x76', '\x30', '\x00', char(img.bytesPerLine()&0xff), char((img.bytesPerLine()>>8)&0xff), char(img.height()&0xff), char((img.height()>>8)&0xff)};
    QByteArray imgData = QByteArray(reinterpret_cast<const char*>(img.bits()),img.sizeInBytes());
    write(str, sizeof(str));
    write( imgData );
    //write("\n");
  }
  return *this;
}

void EscPosPrinter::write(const QByteArray &data)
{
    m_device->write(data);
}

void EscPosPrinter::write(const char *data, int size)
{
    m_device->write(data, size);
}

EscPosPrinter &EscPosPrinter::initialize()
{
    const char str[] = { ESC, '@'};
    qCDebug(EPP) << "init " << QByteArray(str, sizeof(str)) << QByteArray(str, sizeof(str)).toHex();
    write(str, sizeof(str));
    return *this;
}
const char* encodingNames[] = {"" , "?", "IBM 850", "?", "-", "-", "-", "-"
                             , "-", "-", "-", "-", "-", "-", "-", "-"
                             , "-", "IBM 866", "?", "-", "-", "-", "-", "-"
                             , "-", "-", "-", "-", "-", "-", "-", "-"
                             , "-", "-", "-", "-", "-", "-", "-", "ISO8859-2"
                             , "ISO8859-15", "-", "-", "-", "-", "Windows-1250", "Windows-1251", "Windows-1253"
                             , "Windows-1254", "Windows-1255", "Windows-1256", "Windows-1257", "Windows-1258"
                            };
EscPosPrinter &EscPosPrinter::encode(EscPosPrinter::Encoding codec)
{
    const char str[] = { ESC, 't', char(codec)};
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if( size_t(codec)<sizeof(encodingNames)/sizeof(encodingNames[0]) )
      m_codec = QTextCodec::codecForName(encodingNames[codec]);
    else
      m_codec = nullptr;

    if (!m_codec) {
        qCWarning(EPP) << "Could not find a Qt Codec for" << codec;
    }
    qCDebug(EPP) << "encoding" << codec << QByteArray(str, sizeof(str)).toHex() << m_codec;
#else
    if( size_t(codec)<sizeof(encodingNames)/sizeof(encodingNames[0]) )
        m_codec = QStringEncoder(encodingNames[codec]);
    else
        m_codec = QStringEncoder();
    qCDebug(EPP) << "encoding" << codec << QByteArray(str, sizeof(str)) << QByteArray(str, sizeof(str)).toHex() << m_codec.name();
#endif
    write(str, sizeof(str));
    return *this;
}

EscPosPrinter &EscPosPrinter::mode(EscPosPrinter::PrintModes pm)
{
    printModes = pm;
    qCDebug(EPP) << "print modes" << printModes;
    const char str[] = { ESC, '!', char(printModes)};
    write(str, sizeof(str));
    transmitLeftMargin();
    transmitPrintingWidth();
    return *this;
}

EscPosPrinter &EscPosPrinter::modeStandard()
{
    const char str[] = { ESC, 'L'};
    write(str, sizeof(str));
    return *this;
}

EscPosPrinter &EscPosPrinter::modePage()
{
    const char str[] = { ESC, 'S'};
    write(str, sizeof(str));
    return *this;
}

EscPosPrinter &EscPosPrinter::partialCut()
{
    const char str[] = { ESC, 'm'};
    qCDebug(EPP) << "partialCut " << QByteArray(str, sizeof(str)) << QByteArray(str, sizeof(str)).toHex();
    write(str, sizeof(str));
    return *this;
}

EscPosPrinter &EscPosPrinter::totalCut()
{
    const char str[] = { ESC, 'i'};
    qCDebug(EPP) << "totalCut " << QByteArray(str, sizeof(str)) << QByteArray(str, sizeof(str)).toHex();
    write(str, sizeof(str));
    return *this;
}

EscPosPrinter &EscPosPrinter::printAndFeedPaper(quint8 n)
{
    const char str[] = { ESC, 'J', char(n)};
    qCDebug(EPP) << "printAndFeedPaper " << QByteArray(str, sizeof(str)) << QByteArray(str, sizeof(str)).toHex();
    write(str, sizeof(str));
    return *this;
}

EscPosPrinter &EscPosPrinter::presentTicket(quint8 stepCount )//1step == 7.3 mm
{
    const char str[] = { GS, '\x65', '\x03', char(stepCount)};
    qCDebug(EPP) << "presentTicket " << QByteArray(str, sizeof(str)) << QByteArray(str, sizeof(str)).toHex();
    write(str, sizeof(str));
    return *this;
}

EscPosPrinter &EscPosPrinter::align(EscPosPrinter::Justification i)
{
    const char str[] = { ESC, 'a', char(i)};
    qCDebug(EPP) << "justification" << i << QByteArray(str, sizeof(str)) << QByteArray(str, sizeof(str)).toHex();
    write(str, sizeof(str));// TODO doesn't work on DR700
    return *this;
}

EscPosPrinter &EscPosPrinter::paperFeed(int lines)
{
    const char str[] = { ESC, 'd', char(lines)};
    qCDebug(EPP) << "line feeds" << lines << QByteArray(str, sizeof(str)).toHex();
    write(str, sizeof(str));
    return *this;
}

void EscPosPrinter::transmitLeftMargin()
{
  const char str[] = { GS, 'L', char(leftMargin&0xff), char((leftMargin>>8)&0xff)};
  qCDebug(EPP) << "left margin" << leftMargin << QByteArray(str, sizeof(str)).toHex();
  write(str, sizeof(str));
}
void EscPosPrinter::transmitPrintingWidth()
{
  const char str[] = { GS, 'W', char(printingWidth&0xff), char((printingWidth>>8)&0xff)};
  qCDebug(EPP) << "printing width" << printingWidth << QByteArray(str, sizeof(str)).toHex();
  write(str, sizeof(str));
}

EscPosPrinter &EscPosPrinter::setLeftMargin(int value)
{
  leftMargin = value;
  transmitLeftMargin();
  return *this;
}
EscPosPrinter &EscPosPrinter::setPrintingWidth(int value)
{
  printingWidth = value;
  transmitPrintingWidth();
  return *this;
}

int EscPosPrinter::getRawStatus(char mode)
{
  const char str[] = { '\x10', '\x04', mode};
  write(str, sizeof(str));
  QByteArray data = m_device->read(1);
  write(str, sizeof(str)-1 );
  return data[0];

}

EscPosPrinter::PrinterStatuses EscPosPrinter::getStatus()
{
  const char str[] = { '\x10', '\x04', '\x14'};
  write(str, sizeof(str));
  EscPosPrinter::PrinterStatuses result;
  //if(m_device->bytesAvailable()>=6)
  {
    QByteArray data = m_device->read(6);
    if(data.length()>=6)
    {
      qDebug() << int(data[0]) << int(data[1]);
      if(data[2]&0x01)
        result.setFlag( PrinterStatusPaperEnd );
      if(data[2]&0x04)
        result.setFlag( PrinterStatusPaperNearEnd );
      if(data[3]&0x03)
        result.setFlag( PrinterStatusCoverOpened );
      if(data[4]&0x0b || data[5]&0x4d )
        result.setFlag( PrinterStatusOtherDeviceError );
      if(data[4]&0x40)
        result.setFlag( PrinterStatusPaperJam );
    }
    else
      result.setFlag( PrinterStatusNotFound );
  }
  write(str, sizeof(str)-1 );
  return result;
}

EscPosPrinter::QRCode::QRCode(EscPosPrinter::QRCode::Model model, int moduleSize, EscPosPrinter::QRCode::ErrorCorrection erroCorrection, const QByteArray &_data)
{
    qCDebug(EPP) << "QRCode" << model << moduleSize << erroCorrection << _data;

    // Model f165
    // 49 - model1
    // 50 - model2
    // 51 - micro qr code
    const char mT[] = {
        GS, '(', 'k', 0x04, 0x00, 0x31, 0x41, char(model), 0x00};
    data.append(mT, sizeof(mT));

    // Module Size f167
    const char mS[] = {
        GS, '(', 'k', 0x03, 0x00, 0x31, 0x43, char(moduleSize)};
    data.append(mS, sizeof(mS));

    // Error Level f169
    // L = 0, M = 1, Q = 2, H = 3
    const char eL[] = {
        GS, '(', 'k', 0x03, 0x00, 0x31, 0x45, char(erroCorrection)};
    data.append(eL, sizeof(eL));

    // truncate data f180
    int len = _data.length() + 3;// 3 header bytes
    if (len > 7092) {
        len = 7092;
    }

    // data header
    const char dH[] = {
        GS, '(', 'k', char(len), char(len >> 8), 0x31, 0x50, 0x30};
    data.append(dH, sizeof(dH));

    data.append(_data.mid(0, 7092));

    // Print f181
    const char pT[] = {
        GS, '(', 'k', 0x03, 0x00, 0x31, 0x51, 0x30};
    data.append(pT, sizeof(pT));
}

