#include "qhttpclient.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkProxy>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QUrlQuery>
#include <QUrl>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QHttpMultiPart>

QHttpClient::QHttpClient()
{
    mProxy.nType = QCProxy::NONE;
    pNetAcsMgr = new QNetworkAccessManager;
    pMimePart = nullptr;
    pNetReply = nullptr;
    connect(pNetAcsMgr, &QNetworkAccessManager::finished, this, &QHttpClient::onFinished);
    mHeaders["Accept-Language"] = "zh-CN,zh;q=0.9";
    mHeaders["Accept"] = "text/javascript, application/javascript, application/ecmascript, application/x-ecmascript, */*; q=0.01";
    mHeaders["Content-Type"] = "application/x-www-form-urlencoded;charset=utf-8";
    mHeaders["Sec-Fetch-Mode"] = "cors";
    mHeaders["Sec-Fetch-Site"] = "same-origin";
    mHeaders["Sec-Fetch-Dest"] = "empty";
    mHeaders["Accept-Encoding"] = "gzip, deflate, br";
}
QHttpClient::~QHttpClient()
{
    delete pNetAcsMgr;
}
void QHttpClient::SetHeaders(QString k,QString v)
{
    mHeaders[k] = v;
}
void QHttpClient::SetAgent(QString v)
{
    mUA = v;
}
void QHttpClient::AddParam(QString k,QString v)
{
    mParams[k] = v;
}
void QHttpClient::SetProxy(const QCProxy &tgProxy)
{
    mProxy = tgProxy;
}
void QHttpClient::SetContent(const QByteArray& data)
{
    mContent = data;
}
void QHttpClient::AddBoundary(QString szName,QString szValue,ParamAttr dwParamAttr)
{
    if(pMimePart==nullptr)
        pMimePart = new QHttpMultiPart;
    if(dwParamAttr==ParamNormal)
    {
        QHttpPart textPart;
        textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"textField\""));
        textPart.setBody(szValue.toUtf8());
        pMimePart->append(textPart);
    }
    else if(dwParamAttr==ParamFile)
    {
        QHttpPart filePart;
        int pos = szValue.lastIndexOf('.');
        if(pos==-1)
        {
            qDebug() << "filepath format error:" << szValue;
            return;
        }
        QString fmt = szValue.mid(pos+1,-1).toLower();
        if(fmt=="jpeg" || fmt=="jpg" || fmt=="bmp" || fmt=="png")
        {
            filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QString("image/")+fmt));
        }
        else if(fmt=="ofd" || fmt=="pdf")
        {
            filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QString("application/")+fmt));
        }
        else if(fmt=="xlsx" || fmt=="xls")
        {
            filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"));
        }
        filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"" + szName + "\""));
        QFile *file = new QFile(szValue);
        if (!file->open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to open file:" << szValue;
            return;
        }
        filePart.setBodyDevice(file);
        file->setParent(pMimePart); // 当 multiPart 销毁时，file 也将销毁
        pMimePart->append(filePart);
    }
}
QByteArray QHttpClient::Convert(const QMap<QString,QString>& d, QString type)
{
    QByteArray ret;
    if(d.size()==0)
    {
        return ret;
    }
    QMap<QString,QString>::ConstIterator it = d.constBegin();
    if(type.indexOf("json")!=-1)
    {
        QJsonDocument doc;
        QJsonObject obj;
        for(;it!=d.constEnd();it++)
        {
            obj.insert(it.key().toUtf8(),QJsonValue(it.value()));
        }
        doc.setObject(obj);
    }
    else
    {
        for(;it!=d.constEnd();it++)
        {
            ret.append(it.key().toUtf8());
            ret.append("=");
            ret.append(QUrl::toPercentEncoding(it.value().toUtf8(), ":;-_.!~*()"));
        }
    }
    return ret;
}
void QHttpClient::Clear()
{
    mHeaders.clear();
    mParams.clear();
    mContent.clear();
    mUA.clear();
    mProxy.nType = QCProxy::NONE;
    mAnswer.contentType = "";
    mAnswer.content.clear();
    mAnswer.headers.clear();
    if(pMimePart)
    {
        delete pMimePart;
        pMimePart = nullptr;
    }
}
void QHttpClient::onFinished(QNetworkReply *reply)
{
    mAnswer.nCode = reply->error();
    if(reply->error() == QNetworkReply::NoError)
    {
        mAnswer.nCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        mAnswer.content = reply->readAll();
        QList<QNetworkReply::RawHeaderPair> rpHeaders = reply->rawHeaderPairs();
        QList<QNetworkReply::RawHeaderPair>::iterator it = rpHeaders.begin();
        for(;it!=rpHeaders.end();it++)
        {
            mAnswer.headers[it->first] = it->second;
            if(it->first.toLower()=="content-type")
            {
                mAnswer.contentType = it->second;
            }
        }
    }
    reply->deleteLater();
    emit onTranslate(mAnswer);
}
QString QHttpClient::InitRequest(QNetworkRequest *pRequest)
{
    QMap<QString,QString>::iterator itBgn = mHeaders.begin();
    QString contentType;
    for(;itBgn!=mHeaders.end();itBgn++)
    {
        pRequest->setRawHeader(itBgn.key().toUtf8(),itBgn.value().toUtf8());
        QString key = itBgn.key().toLower();
        if(key=="content-type")
            contentType = itBgn.value().toLower();
        else if(key=="user-agent")
            mUA.clear();
    }
    if(!mUA.isEmpty())
        pRequest->setRawHeader("User-Agent",itBgn.value().toUtf8());
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    if(pRequest->url().url().indexOf("https")!=-1)
    {
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        sslConfig.setProtocol(QSsl::TlsV1_2);
    }
    pRequest->setSslConfiguration(sslConfig);
    if(mProxy.nType!=QCProxy::NONE)
    {
        QNetworkProxy proxy;
        if(mProxy.nType==QCProxy::SOCKS5)
            proxy.setType(QNetworkProxy::Socks5Proxy);
        else if(mProxy.nType==QCProxy::HTTP)
            proxy.setType(QNetworkProxy::HttpProxy);
        proxy.setUser(mProxy.sName);
        proxy.setPassword(mProxy.sPass);
        proxy.setPort(mProxy.nPort);
        proxy.setHostName(mProxy.sServer);
        pNetAcsMgr->setProxy(proxy);
    }
    pNetAcsMgr->setRedirectPolicy(QNetworkRequest::SameOriginRedirectPolicy);
    return contentType;
}
QHttpClient::QCReply QHttpClient::RequestGet(QString url,bool bSync,int timeout)
{
    QUrlQuery query;
    QUrl rqURL(url);
    QNetworkRequest request;
    QMap<QString,QString>::iterator it = mParams.begin();
    for(;it!=mParams.end();it++)
    {
        query.addQueryItem(it.key(),it.value());
    }
    rqURL.setQuery(query);
    request.setUrl(rqURL);
    QString contentType = InitRequest(&request);
    pNetAcsMgr->setTransferTimeout(timeout);
    pNetReply = pNetAcsMgr->get(request);
    if(!mParams.isEmpty())
        mParams.clear();
    if(bSync==false)
    {
        QHttpClient::QCReply ret;
        return ret;
    }
    QEventLoop loop;
    connect(pNetReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    onFinished(pNetReply);
    return mAnswer;
}
QHttpClient::QCReply QHttpClient::RequestPost(QString url,bool bSync,int timeout)
{
    QNetworkRequest request(url);
    QString contentType = InitRequest(&request);
    pNetAcsMgr->setTransferTimeout(timeout);
    if(pMimePart)
        pNetReply = pNetAcsMgr->post(request, pMimePart);
    else
        pNetReply = pNetAcsMgr->post(request, Convert(mParams,contentType));
    if(pMimePart)
        delete pMimePart;
    if(!mParams.isEmpty())
        mParams.clear();
    if(bSync==false)
    {
        QHttpClient::QCReply ret;
        return ret;
    }
    QEventLoop loop;
    connect(pNetReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    onFinished(pNetReply);
    return mAnswer;
}
