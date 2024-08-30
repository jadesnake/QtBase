#ifndef QHTTPCLIENT_H
#define QHTTPCLIENT_H

#include <QString>
#include <QVariant>
#include <QMap>
#include <QEventLoop>
#include <QMetaType>
#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;
class QHttpMultiPart;

class QHttpClient : public QObject
{
    Q_OBJECT
public:
    typedef struct _tgProxy
    {
        typedef enum
        {
            HTTP = 0,
            SOCKS5 = 5,
            NONE
        }TYPE;
        QString sName;
        QString sPass;
        TYPE nType;
        QString sServer;
        int nPort;
    }QCProxy;
    typedef struct _tgReply
    {
        int nCode;
        QByteArray content;
        QString contentType;
        QMap<QString,QString> headers;
    }QCReply;
    typedef enum
    {
        ParamNormal,
        ParamFile
    }ParamAttr;
    QHttpClient();
    virtual ~QHttpClient();
    void SetHeaders(QString k,QString v);
    void SetAgent(QString v);
    void AddParam(QString k,QString v);
    void SetProxy(const QCProxy &tgProxy);
    void SetContent(const QByteArray& data);
    void AddBoundary(QString szName,QString szValue,ParamAttr dwParamAttr=ParamNormal);
    void Clear();
    QCReply RequestPost(QString url,bool bSync=false,int timeout=60000);
    QCReply RequestGet(QString url,bool bSync=false,int timeout=60000);
signals:
    void onTranslate(const QCReply& var);
private:
    QByteArray Convert(const QMap<QString,QString>& d, QString type);
    QString InitRequest(QNetworkRequest *pRequest);
private slots:
    void onFinished(QNetworkReply *reply);
private:
    QNetworkAccessManager *pNetAcsMgr;
    QNetworkReply *pNetReply;
    QHttpMultiPart *pMimePart;
    QMap<QString,QString> mHeaders;
    QMap<QString,QString> mParams;
    QByteArray mContent;
    QString mUA;
    QCProxy mProxy;
    QCReply mAnswer;
};

#endif // QHTTPCLIENT_H
