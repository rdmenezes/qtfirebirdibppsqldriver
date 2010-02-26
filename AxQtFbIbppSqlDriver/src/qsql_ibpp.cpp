/*
* This file is part of QtFirebirdIBPPSQLDriver - Qt SQL driver for Firebird with IBPP library
* Copyright (C) 2006-2010 Alex Wencel
*
* Contact e-mail: Alex Wencel <alex.wencel@gmail.com>
* Program URL   : http://code.google.com/p/qtfirebirdibppsqldriver
*
* GNU Lesser General Public License Usage
* This file may be used under the terms of the GNU Lesser
* General Public License version 2.1 as published by the Free Software
* Foundation and appearing in the file LICENSE.LGPL included in the
* packaging of this file.  Please review the following information to
* ensure the GNU Lesser General Public License version 2.1 requirements
* will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
*
* GNU General Public License Usage
* Alternatively, this file may be used under the terms of the GNU
* General Public License version 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in the
* packaging of this file.  Please review the following information to
* ensure the GNU General Public License version 3.0 requirements will be
* met: http://www.gnu.org/copyleft/gpl.html.
*
*/

#include <QTextCodec>
#include <qdatetime.h>
#include <qvariant.h>
#include <qsqlerror.h>
#include <qsqlfield.h>
#include <qsqlindex.h>
#include <qsqlquery.h>
#include <qstringlist.h>
#include <qlist.h>
#include <qvector.h>


#include "ibpp.h"
#include "qsql_ibpp.h"

#define blr_text		(unsigned char)14
#define blr_text2		(unsigned char)15	/* added in 3.2 JPN */
#define blr_short		(unsigned char)7
#define blr_long		(unsigned char)8
#define blr_quad		(unsigned char)9
#define blr_float		(unsigned char)10
#define blr_double		(unsigned char)27
#define blr_d_float		(unsigned char)11
#define blr_timestamp	(unsigned char)35
#define blr_varying		(unsigned char)37
#define blr_varying2	(unsigned char)38	/* added in 3.2 JPN */
#define blr_blob		(unsigned short)261
#define blr_cstring		(unsigned char)40
#define blr_cstring2    (unsigned char)41	/* added in 3.2 JPN */
#define blr_blob_id     (unsigned char)45	/* added from gds.h */
#define blr_sql_date	(unsigned char)12
#define blr_sql_time	(unsigned char)13
#define blr_int64       (unsigned char)16
//-----------------------------------------------------------------------//
static QVariant::Type qIBaseTypeName(int iType)
{
    switch (iType)
    {
    case blr_varying:
    case blr_varying2:
    case blr_text:
    case blr_cstring:
    case blr_cstring2:
        return QVariant::String;
    case blr_sql_time:
        return QVariant::Time;
    case blr_sql_date:
        return QVariant::Date;
    case blr_timestamp:
        return QVariant::DateTime;
    case blr_blob:
        return QVariant::ByteArray;
    case blr_quad:
    case blr_short:
    case blr_long:
        return QVariant::Int;
    case blr_int64:
        return QVariant::LongLong;
    case blr_float:
    case blr_d_float:
    case blr_double:
        return QVariant::Double;
    }
    qWarning("qFBTypeName: unknown datatype: %d", iType);
    return QVariant::Invalid;
}
//-----------------------------------------------------------------------//
static QVariant::Type qIBPPTypeName(int iType)
{
    switch (iType )
    {
    case IBPP::sdString:
        return QVariant::String;
    case IBPP::sdSmallint:
    case IBPP::sdInteger:
        return QVariant::Int;
    case IBPP::sdLargeint:
        return QVariant::LongLong;
    case IBPP::sdFloat:
    case IBPP::sdDouble:
        return QVariant::Double;
    case IBPP::sdTimestamp:
        return QVariant::DateTime;
    case IBPP::sdTime:
        return QVariant::Time;
    case IBPP::sdDate:
        return QVariant::Date;
    case IBPP::sdArray:
        return QVariant::List;
    case IBPP::sdBlob:
        return QVariant::ByteArray;
    default:
        return QVariant::Invalid;
    }
}
//-----------------------------------------------------------------------//
static std::string toIBPPStr(const QString &s)
{
    return std::string(QTextCodec::codecForName("CP1251")->fromUnicode((s)));
}
//-----------------------------------------------------------------------//
static QString fromIBPPStr(std::string &s)
{
    return QTextCodec::codecForName("CP1251")->toUnicode((s.data())).trimmed();
}
//-----------------------------------------------------------------------//
static IBPP::Timestamp toIBPPTimeStamp(const QDateTime &dt)
{
    IBPP::Timestamp ts;
    if (dt.isValid())
        ts = IBPP::Timestamp(dt.date().year(), dt.date().month(), dt.date().day(),
                             dt.time().hour(), dt.time().minute(), dt.time().second(), dt.time().msec());
    return ts;
}
//-----------------------------------------------------------------------//
static QDateTime fromIBPPTimeStamp(IBPP::Timestamp &dt)
{
    int y,m,d,h,min,s,ms;
    dt.GetDate(y,m,d);
    dt.GetTime(h,min,s,ms);
    return QDateTime(QDate(y,m,d), QTime(h,min,s,ms));
}
//-----------------------------------------------------------------------//
static IBPP::Time toIBPPTime(const QTime &t)
{
    IBPP::Time it;
    if (t.isValid())
        it.SetTime(t.hour(), t.minute(), t.second(), t.msec());
    return it;
}
//-----------------------------------------------------------------------//
static QTime fromIBPPTime(IBPP::Time & it)
{
    int h,min,s,ms;
    it.GetTime(h,min,s,ms);
    return QTime(h,min,s,ms);
}
//-----------------------------------------------------------------------//
static IBPP::Date toIBPPDate(const QDate &t)
{
    IBPP::Date id;
    if (t.isValid())
        id.SetDate(t.year(), t.month(), t.day());
    return id;
}
//-----------------------------------------------------------------------//
static QDate fromIBPPDate(IBPP::Date &id)
{
    int y,m,d;
    id.GetDate(y,m,d);
    return QDate(y,m,d);
}
//-----------------------------------------------------------------------//
class QFBDriverPrivate
{
public:
    QFBDriverPrivate(QFBDriver *dd) : d(dd)
    {
        iDb.clear();
        iTr.clear();

        tam = IBPP::amWrite;
        til = IBPP::ilConcurrency;
        tlr = IBPP::lrWait;
        tff = IBPP::TFF(0);
    }

    void setError(const std::string &err, IBPP::Exception &e, QSqlError::ErrorType type);
    void checkTransactionArguments();

public:
    IBPP::Database iDb;
    IBPP::Transaction iTr;
    QList<IBPP::Transaction> iL;

    IBPP::TAM tam;
    IBPP::TIL til;
    IBPP::TLR tlr;
    IBPP::TFF tff;

    QFBDriver* d;
};
//-----------------------------------------------------------------------//
void QFBDriverPrivate::setError(const std::string &err, IBPP::Exception &e, QSqlError::ErrorType type)
{
//	qWarning(err.data());
    d->setLastError(QSqlError(QString::fromLatin1(err.data()),
                              QString::fromLatin1(e.ErrorMessage()), type));
}
//-----------------------------------------------------------------------//
void QFBDriverPrivate::checkTransactionArguments()
{
    if (!d->property("Transaction").isValid())
    {
        tam = IBPP::amWrite;
        til = IBPP::ilConcurrency;
        tlr = IBPP::lrWait;
        tff = IBPP::TFF(0);
        return;
    }

    QString args = d->property("Transaction").toString();
    const QStringList opts(args.split(QLatin1Char(','), QString::SkipEmptyParts));
    for (int i = 0; i < opts.count(); ++i)
    {
        const QString tmp(opts.at(i));
        int idx;
        if ((idx = tmp.indexOf(QLatin1Char('='))) == -1)
        {
            qWarning("QFBDriver::checkTransactionArguments: Illegal option value '%s'",
                     tmp.toLocal8Bit().constData());
            continue;
        }

        const QString opt(tmp.left(idx).simplified());
        const QString val(tmp.mid(idx + 1).simplified());

        if (opt == QLatin1String("TAM"))
        {
            if (val == QLatin1String("amWrite"))
                tam = IBPP::amWrite;
            else
                if (val == QLatin1String("amRead"))
                    tam = IBPP::amRead;
                else
                    qWarning("QFBDriver::checkTransactionArguments: Unknown IBPP::TAM value '%s'",
                             tmp.toLocal8Bit().constData());
            continue;
        }
        else
            if (opt == QLatin1String("TIL"))
            {
                if (val == QLatin1String("ilConcurrency"))
                    til = IBPP::ilConcurrency;
                else
                    if (val == QLatin1String("ilReadDirty"))
                        til = IBPP::ilReadDirty;
                    else
                        if (val == QLatin1String("ilReadCommitted"))
                            til = IBPP::ilReadCommitted;
                        else
                            if (val == QLatin1String("ilConsistency"))
                                til = IBPP::ilConsistency;
                            else
                                qWarning("QFBDriver::checkTransactionArguments: Unknown IBPP::TIL value '%s'",
                                         tmp.toLocal8Bit().constData());
                continue;
            }
            else
                if (opt == QLatin1String("TLR"))
                {
                    if (val == QLatin1String("lrWait"))
                        tlr = IBPP::lrWait;
                    else
                        if (val == QLatin1String("lrNoWait"))
                            tlr = IBPP::lrNoWait;
                        else
                            qWarning("QFBDriver::checkTransactionArguments: Unknown IBPP::TLR value '%s'",
                                     tmp.toLocal8Bit().constData());
                    continue;
                }
                else
                    if (opt == QLatin1String("TFF"))
                    {
                        if (val == QLatin1String("0"))
                            tff = IBPP::TFF(0);
                        else
                            if (val == QLatin1String("tfIgnoreLimbo"))
                                tff = IBPP::tfIgnoreLimbo;
                            else
                                if (val == QLatin1String("tfAutoCommit"))
                                    tff = IBPP::tfAutoCommit;
                                else
                                    if (val == QLatin1String("tfNoAutoUndo"))
                                        tff = IBPP::tfNoAutoUndo;
                                    else
                                        qWarning("QFBDriver::checkTransactionArguments: Unknown IBPP::TLR value '%s'",
                                                 tmp.toLocal8Bit().constData());
                        continue;
                    }
                    else
                        qWarning("QFBDriver::checkTransactionArguments: Unknown transaction attribute '%s'",
                                 tmp.toLocal8Bit().constData());
    }
//    qWarning("IBPP::Transaction arguments(TAM=%d, TIL=%d, TLR=%d, TFF=%d)", tam, til, tlr, tff);
    d->setProperty("Transaction",QVariant());

}
//-----------------------------------------------------------------------//
class QFBResultPrivate
{
public:
    QFBResultPrivate(QFBResult *rr, const QFBDriver *dd);
    ~QFBResultPrivate()
    {
        cleanup();
    }

    void cleanup();

    bool transaction();
    bool commit();

    bool isSelect();

    void setError(const std::string &err, IBPP::Exception &e, QSqlError::ErrorType type);

public:
    QFBResult *r;
    const QFBDriver *d;

    bool localTransaction;

    int queryType;

    IBPP::Database iDb;
    IBPP::Transaction iTr;
    IBPP::Statement iSt;
};
//-----------------------------------------------------------------------//
QFBResultPrivate::QFBResultPrivate(QFBResult *rr, const QFBDriver *dd):
        r(rr), d(dd), queryType(-1)
{
    localTransaction = true;
    iDb = dd->dp->iDb;

    try
    {
        iTr = IBPP::TransactionFactory(iDb);
        iSt = IBPP::StatementFactory(iDb,iTr);
    }
    catch (IBPP::Exception& e)
    {
        setError("Unable create local transaction and statement", e, QSqlError::StatementError);
    }
}
//-----------------------------------------------------------------------//
void QFBResultPrivate::cleanup()
{
    commit();

    //if (!localTransaction)
    //iTr = 0;

    try
    {
        iSt->Close();
    }
    catch (IBPP::Exception& e)
    {
        setError("Unable close statement", e, QSqlError::StatementError);
    }

    queryType = -1;

    r->cleanup();
}
//-----------------------------------------------------------------------//
void QFBResultPrivate::setError(const std::string &err, IBPP::Exception &e, QSqlError::ErrorType type)
{
//	qWarning(err.data());
    qWarning(e.ErrorMessage());
    r->setLastError(QSqlError(QString::fromLatin1(err.data()),
                              QString::fromLatin1(e.ErrorMessage()), type));
}
//-----------------------------------------------------------------------//
bool QFBResultPrivate::isSelect()
{
    bool iss = false;
    try
    {
        iss = (iSt->Type() == IBPP::stSelect);
    }
    catch (IBPP::Exception& e)
    {
        setError("Unable get type of statement", e, QSqlError::StatementError);
    }
    return iss;
}
//-----------------------------------------------------------------------//
bool QFBResultPrivate::transaction()
{
    if (iTr->Started())
        return true;

    if (d->dp->iTr != 0)
        if (d->dp->iTr->Started())
        {
            localTransaction = false;
            iTr.clear();
            iSt.clear();
            iTr = d->dp->iTr;
            iSt = IBPP::StatementFactory(iDb,iTr);
            return true;
        }

    localTransaction = true;

    try
    {
        iTr.clear();
        iSt.clear();
        d->dp->checkTransactionArguments();
        iTr = IBPP::TransactionFactory(iDb, d->dp->tam, d->dp->til, d->dp->tlr, d->dp->tff);
        iSt = IBPP::StatementFactory(iDb,iTr);
        iTr->Start();
    }
    catch (IBPP::Exception& e)
    {
        setError("Unable start transaction", e, QSqlError::TransactionError);
        return false;
    }

    return true;
}
//-----------------------------------------------------------------------//
bool QFBResultPrivate::commit()
{
    if (!localTransaction)
        return true;

    if (!iTr->Started())
        return true;

    try
    {
        iTr->Commit();
    }
    catch (IBPP::Exception& e)
    {
        setError("Unable to commit transaction", e, QSqlError::TransactionError);
        return false;
    }

    return true;
}
//-----------------------------------------------------------------------//
QFBResult::QFBResult(const QFBDriver* db):
        QSqlCachedResult(db)
{
    rp = new QFBResultPrivate(this, db);
}
//-----------------------------------------------------------------------//
QFBResult::~QFBResult()
{
    delete rp;
}
//-----------------------------------------------------------------------//
bool QFBResult::prepare(const QString& query)
{

    if (!driver() || !driver()->isOpen() || driver()->isOpenError())
        return false;

    rp->cleanup();

    setActive(false);
    setAt(QSql::BeforeFirstRow);

    if (!rp->transaction())
    {
        return false;
    }

    try
    {
        rp->iSt->Prepare(toIBPPStr(query));
    }
    catch (IBPP::Exception& e)
    {
        rp->setError("Unable prepare statement", e , QSqlError::StatementError);
        return false;
    }

    setSelect(rp->isSelect());

    return true;
}
//-----------------------------------------------------------------------//
bool QFBResult::exec()
{

    if (!driver() || !driver()->isOpen() || driver()->isOpenError())
        return false;

    if (!rp->transaction())
    {
        return false;
    }


    setActive(false);
    setAt(QSql::BeforeFirstRow);

    int paramCount = 0;

    try
    {
        paramCount = rp->iSt->Parameters();
    }
    catch (IBPP::Exception& e)
    {
        Q_UNUSED(e);
        paramCount = 0;
    }

    bool ok = true;
    if (paramCount)
    {
        QVector<QVariant>& values = boundValues();
        int i;
        if (values.count() > paramCount)
        {
            qWarning("QFBResult::exec: Parameter mismatch, expected %d, got %d parameters",
                     rp->iSt->Parameters(), values.count());
            return false;
        }
        for (i = 1; i <= values.count(); ++i)
        {

            if (!rp->iSt->ParameterType(i))
                continue;

            const QVariant val(values[i-1]);

            if (val.isNull())
            {
                try
                {
                    rp->iSt->SetNull(i);
                }
                catch (IBPP::Exception& e)
                {
                    rp->setError("Unable to set NULL", e, QSqlError::StatementError);
                    return false;
                }
                continue;
            }

            switch (rp->iSt->ParameterType(i))
            {
            case IBPP::sdLargeint:
                if (rp->iSt->ParameterScale(i))
                    rp->iSt->Set(i, val.toDouble());
                else
                    rp->iSt->Set(i, val.toLongLong());
                break;
            case IBPP::sdInteger:
                if (rp->iSt->ParameterScale(i))
                    rp->iSt->Set(i, val.toDouble());
                else
                    rp->iSt->Set(i, val.toInt());
                break;
            case IBPP::sdSmallint:
                if (rp->iSt->ParameterScale(i))
                    rp->iSt->Set(i, val.toDouble());
                else
                    rp->iSt->Set(i, (short)val.toInt());
                break;
            case IBPP::sdFloat:
                rp->iSt->Set(i, (float)val.toDouble());
                break;
            case IBPP::sdDouble:
                rp->iSt->Set(i, val.toDouble());
                break;
            case IBPP::sdTimestamp:
                rp->iSt->Set(i, toIBPPTimeStamp(val.toDateTime()));
                break;
            case IBPP::sdTime:
                rp->iSt->Set(i, toIBPPTime(val.toTime()));
                break;
            case IBPP::sdDate:
                rp->iSt->Set(i, toIBPPDate(val.toDate()));
                break;
            case IBPP::sdString:
                rp->iSt->Set(i, toIBPPStr(val.toString()));
                break;
            case IBPP::sdBlob:
                {
                    std::string  ss;
                    QByteArray ba = val.toByteArray();
                    ss.resize(ba.size());
                    ss.assign(ba.constData(), ba.size());
                    rp->iSt->Set(i, ss);
                    break;
                }
            case IBPP::sdArray:
//                ok &= rp->writeArray(i, val.toList());
                break;
            default:
                qWarning("QFBResult::exec: Unknown datatype %d",
                         rp->iSt->ParameterType(i));
                ok = false;
                break;
            }
        }
    }

    if (!ok)
        return false;

    try
    {
        rp->iSt->Execute();
    }
    catch (IBPP::Exception& e)
    {
        rp->setError("Unable execute statement", e ,QSqlError::StatementError);
        return false;
    }
    int cols = 0;
    try
    {
        cols = rp->iSt->Columns();
    }
    catch (IBPP::Exception& e)
    {
        Q_UNUSED(e);
    }

    if (cols > 0)
        init(cols);
    else
        cleanup();

    if (! rp->isSelect())
        rp->commit();

    setActive(true);
    return true;
}
//-----------------------------------------------------------------------//
bool QFBResult::reset (const QString& query)
{
    if (!prepare(query))
        return false;
    return exec();
}
//-----------------------------------------------------------------------//
bool QFBResult::gotoNext(QSqlCachedResult::ValueCache& row, int rowIdx)
{

    bool stat;
    try
    {
        stat = rp->iSt->Fetch();
    }
    catch (IBPP::Exception& e)
    {
        rp->setError("Could not fetch next item", e, QSqlError::StatementError);
        return false;
    }

    if (!stat)
    {
        // no more rows
        setAt(QSql::AfterLastRow);
        return false;
    }

    if (rowIdx < 0) // not interested in actual values
        return true;

    int cols = 0;
    try
    {
        cols = rp->iSt->Columns();
    }
    catch (IBPP::Exception& e)
    {
        Q_UNUSED(e);
    }

    for (int i = 1; i <= cols; ++i)
    {
        int idx = rowIdx + i - 1;

        if (rp->iSt->IsNull(i))
        {
            // null value
            QVariant v;
            v.convert(qIBPPTypeName(rp->iSt->ColumnType(i)));
            row[idx] = v;
            continue;
        }

        switch (rp->iSt->ColumnType(i))
        {
        case IBPP::sdDate:
            {
                IBPP::Date dt;
                rp->iSt->Get(i, dt);
                row[idx] = fromIBPPDate(dt);
                break;
            }
        case IBPP::sdTime:
            {
                IBPP::Time tm;
                rp->iSt->Get(i, tm);
                row[idx] = fromIBPPTime(tm);
                break;
            }
        case IBPP::sdTimestamp:
            {
                IBPP::Timestamp ts;
                rp->iSt->Get(i, ts);
                row[idx] = fromIBPPTimeStamp(ts);
                break;
            }
        case IBPP::sdSmallint:
            {
                if (rp->iSt->ColumnScale(i))
                {
                    double l_Double;
                    rp->iSt->Get(i, l_Double);
                    row[idx] = l_Double;
                }
                else
                {
                    short l_Short;
                    rp->iSt->Get(i, l_Short);
                    row[idx] =l_Short;
                }
                break;
            }
        case IBPP::sdInteger:
            {
                if (rp->iSt->ColumnScale(i))
                {
                    double l_Double;
                    rp->iSt->Get(i, l_Double);
                    row[idx] = l_Double;
                }
                else
                {
                    int l_Integer;
                    rp->iSt->Get(i, l_Integer);
                    row[idx] = l_Integer;
                }
                break;
            }
        case IBPP::sdLargeint:
            {
                if (rp->iSt->ColumnScale(i))
                {
                    double l_Double;
                    rp->iSt->Get(i, l_Double);
                    row[idx] = l_Double;
                }
                else
                {
                    qlonglong l_Long;
                    rp->iSt->Get(i, l_Long);
                    row[idx] = l_Long;

                }
                break;
            }
        case IBPP::sdFloat:
            {
                float l_Float;
                rp->iSt->Get(i, l_Float);
                row[idx] = l_Float;
                break;
            }
        case IBPP::sdDouble:
            {
                double l_Double;
                rp->iSt->Get(i, l_Double);
                row[idx] = l_Double;
                break;
            }
        case IBPP::sdString:
            {
                std::string l_String;
                rp->iSt->Get(i, l_String);
                row[idx] = fromIBPPStr(l_String);
                break;
            }
        case IBPP::sdArray:
            {
//	            row[idx] = rp->fetchArray(i, (ISC_QUAD*)buf);
                break;
            }
        case IBPP::sdBlob:
            {
                IBPP::Blob l_Blob = IBPP::BlobFactory(rp->iDb, rp->iTr);
                rp->iSt->Get(i, l_Blob);

                QByteArray l_QBlob;

                l_Blob->Open();
                int l_Read, l_Offset = 0;
                char buffer[1024];
                while ((l_Read = l_Blob->Read(buffer, 1024)))
                {
                    l_QBlob.resize(l_QBlob.size() + l_Read);
                    memcpy(l_QBlob.data() + l_Offset, buffer, l_Read);
                    l_Offset += l_Read;
                }
                l_Blob->Close();

                row[idx] = l_QBlob;
                break;
            }
        default:
            row[idx] =  QVariant();
            break;
        }
    }

    return true;
}
//-----------------------------------------------------------------------//
int QFBResult::size()
{
    int nra = -1;
    return nra;
    // :(
    if (isSelect())
        return nra;
    try
    {
        nra = rp->iSt->AffectedRows();
    }
    catch (IBPP::Exception& e)
    {
        Q_UNUSED(e);
    }
    return nra;
}
//-----------------------------------------------------------------------//
int QFBResult::numRowsAffected()
{
    int nra = -1;
    if (isSelect())
        return nra;

    try
    {
        nra = rp->iSt->AffectedRows();
    }
    catch (IBPP::Exception& e)
    {
        Q_UNUSED(e);
    }
    return nra;
}
//-----------------------------------------------------------------------//
QSqlRecord QFBResult::record() const
{
    QSqlRecord rec;
    if (!isActive())
        return rec;

    int cols = 0;
    try
    {
        cols = rp->iSt->Columns();
    }
    catch (IBPP::Exception& e)
    {
        Q_UNUSED(e);
    }
    for (int i = 1; i <= cols; ++i)
    {

        QSqlField f(QString::fromLatin1(rp->iSt->ColumnAlias(i)).simplified(),
                    qIBPPTypeName(rp->iSt->ColumnType(i)));
        f.setLength(rp->iSt->ColumnSize(i));
        f.setPrecision(rp->iSt->ColumnScale(i));
        f.setSqlType(rp->iSt->ColumnType(i));
        rec.append(f);
    }
    return rec;
}
//-----------------------------------------------------------------------//
QVariant QFBResult::handle() const
{
    return QVariant(qRegisterMetaType<IBPP::IStatement *>("ibpp_statement_handle"), rp->iSt.intf());
    return QVariant();
}
//-----------------------------------------------------------------------//
//-----------------------------------------------------------------------//
QFBDriver::QFBDriver(QObject * parent)
        : QSqlDriver(parent)
{
    dp = new QFBDriverPrivate(this);
}
//-----------------------------------------------------------------------//
QFBDriver::QFBDriver(void *connection, QObject *parent)
        : QSqlDriver(parent)
{
    dp = new QFBDriverPrivate(this);
    dp->iDb=(IBPP::IDatabase*)connection;
    setOpen(true);
    setOpenError(false);
}
//-----------------------------------------------------------------------//
QFBDriver::~QFBDriver()
{
    delete dp;
}
//-----------------------------------------------------------------------//
bool QFBDriver::hasFeature(DriverFeature f) const
{
    switch (f)
    {
    case Transactions:
    case PreparedQueries:
    case PositionalPlaceholders:
    case Unicode:
    case BLOB:
        return true;
    default:
        return false;
    }
    return false;
}
//-----------------------------------------------------------------------//
bool QFBDriver::open(const QString & db,
                     const QString & user,
                     const QString & password,
                     const QString & host,
                     int /*port*/,
                     const QString & connOpts )
{

    QString charSet = QLatin1String("NONE");
    QString role = QLatin1String("");

    // Set connection attributes
    const QStringList opts(connOpts.split(QLatin1Char(';'), QString::SkipEmptyParts));
    for (int i = 0; i < opts.count(); ++i)
    {
        const QString tmp(opts.at(i));
        int idx;
        if ((idx = tmp.indexOf(QLatin1Char('='))) == -1)
        {
            qWarning("QFBDriver::open: Illegal connect option value '%s'",
                     tmp.toLocal8Bit().constData());
            continue;
        }

        const QString opt(tmp.left(idx));
        const QString val(tmp.mid(idx + 1).simplified());

        if (opt == QLatin1String("CHARSET"))
        {
            charSet = val;
        }
        else if (opt == QLatin1String("ROLE"))
        {
            role = val;
        }
        else
        {
            qWarning("QFBDriver::open: Unknown connection attribute '%s'",
                     tmp.toLocal8Bit().constData());
        }
    }

    if (charSet.isEmpty())
    {
        qWarning("QFBDriver::open: set database charset");
        return false;
    }

    if (isOpen())
        close();

    try
    {
        dp->iDb=IBPP::DatabaseFactory(host.toStdString(),
                                      db.toStdString(),
                                      user.toStdString(),
                                      password.toStdString(),
                                      role.toStdString(),
                                      charSet.toStdString(),
                                      "");

        dp->iDb->Connect();

    }
    catch (IBPP::Exception& e)
    {
        setOpenError(true);
        dp->setError("Unable to connect", e, QSqlError::ConnectionError);
        return false;
    }

    setOpen(true);
    return true;
}
//-----------------------------------------------------------------------//
void QFBDriver::close()
{
    if (!isOpen())
        return;

    if (dp->iL.count())
        qWarning("QFBDriver::close : %d transaction still sarted ! Rollback all.",dp->iL.count());

    try
    {
        dp->iDb->Disconnect();
    }
    catch (IBPP::Exception& e)
    {
        dp->setError("Unable to disconnect", e, QSqlError::ConnectionError);
        return;
    }
    setOpen(false);
    setOpenError(false);
}
//-----------------------------------------------------------------------//
QSqlResult *QFBDriver::createResult() const
{
    return new QFBResult(this);
}
//-----------------------------------------------------------------------//
bool QFBDriver::beginTransaction()
{
    if (!isOpen() || isOpenError())
        return false;

    //if (dp->iTr != 0)
    //if (dp->iTr->Started())
    //return false;

    dp->iTr.clear();

    try
    {
        dp->checkTransactionArguments();
        dp->iTr = IBPP::TransactionFactory(dp->iDb, dp->tam, dp->til, dp->tlr, dp->tff);
        dp->iTr->Start();
    }
    catch (IBPP::Exception& e)
    {
        dp->iTr.clear();
        dp->setError("Unable start transaction", e, QSqlError::TransactionError);
        return false;
    }

    dp->iL.push_back(dp->iTr);
    if (dp->iL.count() > 1)
        qWarning("QFBDriver::transaction : Start transactions  %d.",dp->iL.count());

    return true;
}
//-----------------------------------------------------------------------//
bool QFBDriver::commitTransaction()
{
    if (!isOpen() || isOpenError())
        return false;
    if (dp->iTr == 0)
        return false;

    try
    {
        dp->iTr->Commit();
    }
    catch (IBPP::Exception& e)
    {
        dp->setError("Unable to commit transaction", e, QSqlError::TransactionError);
        return false;
    }

    dp->iTr.clear();
    dp->iL.removeLast ();
    if (!dp->iL.isEmpty())
    {
        dp->iTr = dp->iL.last();
    }

    return true;
}
//-----------------------------------------------------------------------//
bool QFBDriver::rollbackTransaction()
{
    if (!isOpen() || isOpenError())
        return false;
    if (dp->iTr == 0)
        return false;

    try
    {
        dp->iTr->Rollback();
    }
    catch (IBPP::Exception& e)
    {
        dp->setError("Unable to rollback transaction" , e , QSqlError::TransactionError);
        return false;
    }

    dp->iTr.clear();
    dp->iL.removeLast ();
    if (!dp->iL.isEmpty())
    {
        dp->iTr = dp->iL.last();
    }
    return true;
}
//-----------------------------------------------------------------------//
QStringList QFBDriver::tables(QSql::TableType type) const
{
    QStringList res;
    if (!isOpen())
        return res;

    QString typeFilter;

    if (type == QSql::SystemTables)
    {
        typeFilter += QLatin1String("RDB$SYSTEM_FLAG != 0");
    }
    else if (type == (QSql::SystemTables | QSql::Views))
    {
        typeFilter += QLatin1String("RDB$SYSTEM_FLAG != 0 OR RDB$VIEW_BLR NOT NULL");
    }
    else
    {
        if (!(type & QSql::SystemTables))
            typeFilter += QLatin1String("RDB$SYSTEM_FLAG = 0 AND ");
        if (!(type & QSql::Views))
            typeFilter += QLatin1String("RDB$VIEW_BLR IS NULL AND ");
        if (!(type & QSql::Tables))
            typeFilter += QLatin1String("RDB$VIEW_BLR IS NOT NULL AND ");
        if (!typeFilter.isEmpty())
            typeFilter.chop(5);
    }
    if (!typeFilter.isEmpty())
        typeFilter.prepend(QLatin1String("where "));

    QSqlQuery q(createResult());
    q.setForwardOnly(true);
    if (!q.exec(QLatin1String("select rdb$relation_name from rdb$relations ") + typeFilter))
        return res;
    while (q.next())
        res << q.value(0).toString().simplified();

    return res;
}
//-----------------------------------------------------------------------//
QSqlRecord QFBDriver::record(const QString& tablename) const
{
    QSqlRecord rec;
    if (!isOpen())
        return rec;

    QSqlQuery q(createResult());
    q.setForwardOnly(true);

    q.exec(QLatin1String("SELECT a.RDB$FIELD_NAME, b.RDB$FIELD_TYPE, b.RDB$FIELD_LENGTH, "
                         "b.RDB$FIELD_SCALE, b.RDB$FIELD_PRECISION, a.RDB$NULL_FLAG "
                         "FROM RDB$RELATION_FIELDS a, RDB$FIELDS b "
                         "WHERE b.RDB$FIELD_NAME = a.RDB$FIELD_SOURCE "
                         "AND a.RDB$RELATION_NAME = '") + tablename.toUpper() + QLatin1String("' "
                                 "ORDER BY a.RDB$FIELD_POSITION"));

    while (q.next())
    {
        int type = q.value(1).toInt();
        QSqlField f(q.value(0).toString().simplified(), qIBaseTypeName(type));
        f.setLength(q.value(2).toInt()); // ?????????
        f.setPrecision(qAbs(q.value(3).toInt()));
        f.setRequired(q.value(5).toInt() > 0 ? true : false);
        f.setSqlType(type);

        rec.append(f);
    }
    return rec;
}
//-----------------------------------------------------------------------//
QSqlIndex QFBDriver::primaryIndex(const QString &table) const
{
    QSqlIndex index(table);
    if (!isOpen())
        return index;

    QSqlQuery q(createResult());
    q.setForwardOnly(true);
    q.exec(QLatin1String("SELECT a.RDB$INDEX_NAME, b.RDB$FIELD_NAME, d.RDB$FIELD_TYPE "
                         "FROM RDB$RELATION_CONSTRAINTS a, RDB$INDEX_SEGMENTS b, RDB$RELATION_FIELDS c, RDB$FIELDS d "
                         "WHERE a.RDB$CONSTRAINT_TYPE = 'PRIMARY KEY' "
                         "AND a.RDB$RELATION_NAME = '") + table.toUpper() +
           QLatin1String(" 'AND a.RDB$INDEX_NAME = b.RDB$INDEX_NAME "
                         "AND c.RDB$RELATION_NAME = a.RDB$RELATION_NAME "
                         "AND c.RDB$FIELD_NAME = b.RDB$FIELD_NAME "
                         "AND d.RDB$FIELD_NAME = c.RDB$FIELD_SOURCE "
                         "ORDER BY b.RDB$FIELD_POSITION"));

    while (q.next())
    {
        QSqlField field(q.value(1).toString().simplified(), qIBaseTypeName(q.value(2).toInt()));
        index.append(field); //TODO: asc? desc?
        index.setName(q.value(0).toString());
    }

    return index;
}
//-----------------------------------------------------------------------//
QString QFBDriver::formatValue(const QSqlField &field, bool trimStrings) const
{
    switch (field.type())
    {
    case QVariant::DateTime:
        {
            QDateTime datetime = field.value().toDateTime();
            if (datetime.isValid())
                return QLatin1Char('\'') + datetime.toString(QString::fromLatin1("dd.MM.yyyy hh:mm:ss")) +
                       QLatin1Char('\'');
            else
                return QLatin1String("NULL");
        }
    case QVariant::Time:
        {
            QTime time = field.value().toTime();
            if (time.isValid())
                return QLatin1Char('\'') + time.toString(QString::fromLatin1("hh:mm:ss")) +
                       QLatin1Char('\'');
            else
                return QLatin1String("NULL");
        }
    case QVariant::Date:
        {
            QDate date = field.value().toDate();
            if (date.isValid())
                return QLatin1Char('\'') + date.toString(QString::fromLatin1("dd.MM.yyyy")) +
                       QLatin1Char('\'');
            else
                return QLatin1String("NULL");
        }
    default:
        return QSqlDriver::formatValue(field, trimStrings);
    }
}
//-----------------------------------------------------------------------//
QVariant QFBDriver::handle() const
{
    return QVariant(qRegisterMetaType<IBPP::IDatabase *>("ibbp_db_handle"), dp->iDb.intf());
    return QVariant();
}
//-----------------------------------------------------------------------//
