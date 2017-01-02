/*
    Copyright (C) 2008-2016 Andres Cabrera
    mantaraya36@gmail.com

    This file is part of CsoundQt.

    CsoundQt is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    CsoundQt is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/


// code based partly on CsoundHtmlWrapper by Michael Gogins https://github.com/gogins/gogins.github.io/tree/master/csound_html5

#include "csoundhtmlwrapper.h"
#include <QApplication>
#include <QDebug>

CsoundHtmlWrapper::CsoundHtmlWrapper(QObject *parent) :
    QObject(parent),
    csound_stop(true),
    csound_finished(true),
    csound_thread(nullptr),
    csound(nullptr),
    m_csoundEngine(nullptr),
#if !defined(_MSC_VER)
    csound_score_queue(0),
    csound_event_queue(0),
#endif
    message_callback(nullptr)
{
#if !defined(_MSC_VER)
    csound_score_queue = new boost::lockfree::queue<char *, boost::lockfree::fixed_sized<false> >(0);
    csound_event_queue= new boost::lockfree::queue<ScoreEvent *, boost::lockfree::fixed_sized<false> >(0);
#endif
}

CsoundHtmlWrapper::~CsoundHtmlWrapper() {
    ScoreEvent *event = 0;
    char *score_text = 0;
#if defined(_MSC_VER)
    while (csound_event_queue.try_pop(event)) {
#else
    while (csound_event_queue->pop(event)) {
#endif
        delete event;
    }
#if defined(_MSC_VER)
    while (csound_score_queue.try_pop(score_text)) {
#else
    while (csound_score_queue->pop(score_text)) {
#endif
        free(score_text);
    }
#if !defined(_MSC_VER)
    if (csound_score_queue) {
        delete csound_score_queue;
        csound_score_queue = 0;

    }
    if (csound_event_queue) {
        delete csound_event_queue;
        csound_event_queue = 0;

    }
#endif
}

void CsoundHtmlWrapper::setCsoundEngine(CsoundEngine *csEngine)
{
    m_csoundEngine = csEngine;
	if (m_csoundEngine) {
        csound = m_csoundEngine->getCsound();
    }
}

int CsoundHtmlWrapper::compileCsd(const QString &filename) {
    if (!csound) {
        return -1;
    }
#if CS_APIVERSION>=4
    return csoundCompileCsd(csound, filename.toLocal8Bit());
#else
    return csoundCompileCsd(csound, filename.toLocal8Bit().data());
#endif
}

int CsoundHtmlWrapper::compileCsdText(const QString &text) {
    if (!csound) {
        return -1;
    }
    return csoundCompileCsdText(csound, text.toLocal8Bit());
}

int CsoundHtmlWrapper::compileOrc(const QString &text) {
    if (!csound) {
        return -1;
    }
    return csoundCompileOrc(csound, text.toLocal8Bit());
}

double CsoundHtmlWrapper::evalCode(const QString &text) {
    if (!csound) {
        return -1;
    }
    return csoundEvalCode(csound, text.toLocal8Bit());
}

double CsoundHtmlWrapper::get0dBFS() {
    if (!csound) {
        return -1;
    }
    return csoundGet0dBFS(csound); //cs->Get0dBFS();
}

int CsoundHtmlWrapper::getApiVersion() {
    if (!csound) {
        return -1;
    }
    return csoundGetAPIVersion();
}

double CsoundHtmlWrapper::getControlChannel(const QString &name) {
    if (!csound) {
        return -1;
    }
    int result = 0;
    double value = csoundGetControlChannel(csound, name.toLocal8Bit(), &result);
    return value;
}

qint64 CsoundHtmlWrapper::getCurrentTimeSamples() { // FIXME: unknown type int64_t qint64
    if (!csound) {
        return -1;
    }
    return csoundGetCurrentTimeSamples(csound);
}

QString CsoundHtmlWrapper::getEnv(const QString &name) { // not sure, if it works... test with setGlobalEnv
    if (!csound) {
        return QString();
    }
    return csoundGetEnv(csound,name.toLocal8Bit());
}

int CsoundHtmlWrapper::getKsmps() {
    if (!csound) {
        return -1;
    }
    return csoundGetKsmps(csound);
}

int CsoundHtmlWrapper::getNchnls() {
    if (!csound) {
        return -1;
    }
    return csoundGetNchnls(csound);
}

int CsoundHtmlWrapper::getNchnlsInput() {
    if (!csound) {
        return -1;
    }
    return csoundGetNchnlsInput(csound);
}

QString CsoundHtmlWrapper::getOutputName() {
    if (!csound) {
        return QString();
    }
    return QString(csoundGetOutputName(csound));
}

double CsoundHtmlWrapper::getScoreOffsetSeconds() {
    if (!csound) {
        return -1;
    }
    return csoundGetScoreOffsetSeconds(csound);
}

double CsoundHtmlWrapper::getScoreTime() {
    if (!csound) {
        return -1;
    }
    return csoundGetScoreTime(csound);
}

int CsoundHtmlWrapper::getSr() {
    if (!csound) {
        return -1;
    }
    return csoundGetSr(csound);
}

QString CsoundHtmlWrapper::getStringChannel(const QString &name) {
    if (!csound) {
        return QString();
    }
    char buffer[0x100];
    csoundGetStringChannel(csound,name.toLocal8Bit(), buffer);
    return QString(buffer);
}

int CsoundHtmlWrapper::getVersion() {
    return csoundGetVersion();
}

bool CsoundHtmlWrapper::isPlaying() {
    if (!m_csoundEngine)
        return false;
    else
        return m_csoundEngine->isRunning();
}

int CsoundHtmlWrapper::isScorePending() {
    if (!csound) {
        return -1;
    }
    return csoundIsScorePending(csound);
}

void CsoundHtmlWrapper::message(const QString &text) {
    if (!csound) {
        return;
    }
    csoundMessage(csound, text.toLocal8Bit());
}

int CsoundHtmlWrapper::perform() {
    if (!csound) {
        return -1;
    }
    csound_thread = new std::thread(&CsoundHtmlWrapper::perform_thread_routine, this);
    if (csound_thread) {
        return 0;
    } else {
        return 1;
    }
}

int CsoundHtmlWrapper::perform_thread_routine() {
    qDebug() ;
    int result = 0;
    message("Csound has started running...\n");
    qDebug("Csound has started: result: %d", result);
    ScoreEvent *event = 0;
    char *score_text = 0;
    int kperiods = 0;
    csound_stop = false;
    csound_finished = 0;
    while (true) {
        if (csound_stop == true) {
            break;
        }
#if defined(_MSC_VER)
        while (csound_event_queue.try_pop(event)) {
#else
        while (csound_event_queue->pop(event)) {
#endif
            csoundScoreEvent(csound, event->opcode, event->pfields.data(), event->pfields.size());
            delete event;
        }
#if defined(_MSC_VER)
        while (csound_score_queue.try_pop(score_text)) {
#else
        while (csound_score_queue->pop(score_text)) {
#endif
            csoundReadScore(csound, score_text);
            free(score_text);
        }
        csound_finished = csoundPerformKsmps(csound);
        if (csound_finished != 0) {
            break;
        }
    }
    message("Csound has stopped running.\n");
    qDebug("Csound has stopped: kperiods: %d csound_stop: %d csound_finished: %d csound: %p", kperiods, csound_stop, csound_finished, csound);
#if defined(_MSC_VER)
    while (csound_event_queue.try_pop(event)) {
#else
    while (csound_event_queue->pop(event)) {
#endif
        delete event;
    }
#if defined(_MSC_VER)
    while (csound_score_queue.try_pop(score_text)) {
#else
    while (csound_score_queue->pop(score_text)) {
#endif
        free(score_text);
    }
    // Although the thread has been started by the CsoundHtmlWrapper,
    // this cleanup should be done by the CsoundEngine.
    // result = csoundCleanup(csound_);
    // csoundReset(csound_);
    return csound_finished;
}


int CsoundHtmlWrapper::readScore(const QString &text) {
    if (!csound) {
        return -1;
    }
#if defined(_MSC_VER)
    csound_score_queue.push(strdup(text.toLocal8Bit()));
#else
    csound_score_queue->push(strdup(text.toLocal8Bit()));
#endif
    return 0;
}

void CsoundHtmlWrapper::reset() {
    if (!csound) {
        return;
    }
    csoundReset(csound);
}

void CsoundHtmlWrapper::rewindScore() {
    if (!csound) {
        return;
    }
    csoundRewindScore(csound);
}

int CsoundHtmlWrapper::runUtility(const QString &command, int argc, char **argv) {
    if (!csound) {
        return -1;
    }
    return csoundRunUtility(csound, command.toLocal8Bit(), argc, argv); // probably does not work from JS due char **
}

int CsoundHtmlWrapper::scoreEvent(char opcode, const double *pfields, long field_count) {
    if (!csound) {
        return -1;
    }
    ScoreEvent *event = new ScoreEvent;
    event->opcode = opcode;
    for(int i = 0; i < field_count; i++) {
        event->pfields.push_back(pfields[i]);
    }
#if defined(_MSC_VER)
    csound_event_queue.push(event);
#else
    csound_event_queue->push(event);
#endif
    return 0;
}

void CsoundHtmlWrapper::setControlChannel(const QString &name, double value) {
    if (!csound) {
        return;
    }
    csoundSetControlChannel(csound,name.toLocal8Bit(), value);
}

int CsoundHtmlWrapper::setGlobalEnv(const QString &name, const QString &value) {
    if (!csound) {
        return -1;
    }
    return csoundSetGlobalEnv(name.toLocal8Bit(), value.toLocal8Bit());
}

void CsoundHtmlWrapper::setInput(const QString &name){
    if (!csound) {
        return;
    }
#if CS_APIVERSION>=4
    csoundSetInput(csound, name.toLocal8Bit());
#else
    csoundSetInput(csound, name.toLocal8Bit().data());
#endif
}

void CsoundHtmlWrapper::setMessageCallback(QObject *callback){
    qDebug();
    callback->dumpObjectInfo();
}

int CsoundHtmlWrapper::setOption(const QString &name){
    if (!csound) {
        return -1;
    }
#if CS_APIVERSION>=4
    return csoundSetOption(csound, name.toLocal8Bit());
#else
    return csoundSetOption(csound, name.toLocal8Bit().data());
#endif
}

void CsoundHtmlWrapper::setOutput(const QString &name, const QString &type, const QString &format){
    if (!csound) {
        return;
    }
#if CS_APIVERSION>=4
    csoundSetOutput(csound, name.toLocal8Bit(), type.toLocal8Bit(), format.toLocal8Bit());
#else
    csoundSetOutput(csound, name.toLocal8Bit().data(), type.toLocal8Bit().data(), format.toLocal8Bit().data());
#endif
}

void CsoundHtmlWrapper::setScoreOffsetSeconds(double value){
    if (!csound) {
        return;
    }
    csoundSetScoreOffsetSeconds(csound, value);
}

void CsoundHtmlWrapper::setScorePending(bool value){
    if (!csound) {
        return;
    }
    csoundSetScorePending(csound,(int) value);
}

void CsoundHtmlWrapper::setStringChannel(const QString &name, const QString &value){
    if (!csound) {
        return;
    }
    csoundSetStringChannel(csound,  name.toLocal8Bit(), value.toLocal8Bit().data());
}

int CsoundHtmlWrapper::start(){
    int result = 0;
    if (!csound) {
        return -1;
    }
    result = csoundStart(csound);
    return result;
}

void CsoundHtmlWrapper::stop(){
    if (!csound) {
        return;
    }
    csound_stop = true;
    if (csound_thread) {
        csound_thread->join();
        csound_thread = nullptr;
    }
    // Although the thread has been started in the CsoundHtmlWrapper,
    // the actual cleanup should be done by the CsoundEngine.
    // csoundStop(csound);
}

double CsoundHtmlWrapper::tableGet(int table_number, int index){
    if (!csound) {
        return -1;
    }
    return csoundTableGet(csound, table_number, index);
}

int CsoundHtmlWrapper::tableLength(int table_number){
    if (!csound) {
        return -1;
    }
    return csoundTableLength(csound, table_number);
}

void CsoundHtmlWrapper::tableSet(int table_number, int index, double value){
    if (!csound) {
        return;
    }
    csoundTableSet(csound, table_number, index, value);
}



