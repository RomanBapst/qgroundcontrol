/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#include "MissionManagerTest.h"
#include "LinkManager.h"
#include "MultiVehicleManager.h"

UT_REGISTER_TEST(MissionManagerTest)

const MissionManagerTest::TestCase_t MissionManagerTest::_rgTestCases[] = {
    { "1\t0\t3\t16\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_WAYPOINT,     10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "1\t0\t3\t17\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_UNLIM, 10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "1\t0\t3\t18\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_TURNS, 10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "1\t0\t3\t19\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_TIME,  10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "1\t0\t3\t21\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LAND,         10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "1\t0\t3\t22\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_TAKEOFF,      10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "1\t0\t2\t112\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n", { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_CONDITION_DELAY,  10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_MISSION } },
    { "1\t0\t2\t177\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n", { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_DO_JUMP,          10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_MISSION } },
};

MissionManagerTest::MissionManagerTest(void)
    : _mockLink(NULL)
{
    
}

void MissionManagerTest::init(void)
{
    UnitTest::init();
    
    LinkManager* linkMgr = LinkManager::instance();
    Q_CHECK_PTR(linkMgr);
    
    _mockLink = new MockLink();
    Q_CHECK_PTR(_mockLink);
    LinkManager::instance()->_addLink(_mockLink);
    
    linkMgr->connectLink(_mockLink);
    
    // Wait for the Vehicle to work it's way through the various threads
    
    QSignalSpy spyVehicle(MultiVehicleManager::instance(), SIGNAL(activeVehicleChanged(Vehicle*)));
    QCOMPARE(spyVehicle.wait(5000), true);
    
    // Wait for the Mission Manager to finish it's initial load
    
    _missionManager = MultiVehicleManager::instance()->activeVehicle()->missionManager();
    QVERIFY(_missionManager);
    
    _rgSignals[canEditChangedSignalIndex] =             SIGNAL(canEditChanged(bool));
    _rgSignals[newMissionItemsAvailableSignalIndex] =   SIGNAL(newMissionItemsAvailable(void));
    _rgSignals[inProgressChangedSignalIndex] =          SIGNAL(inProgressChanged(bool));
    _rgSignals[errorSignalIndex] =                      SIGNAL(error(int, const QString&));

    _multiSpy = new MultiSignalSpy();
    Q_CHECK_PTR(_multiSpy);
    QCOMPARE(_multiSpy->init(_missionManager, _rgSignals, _cSignals), true);
    
    if (_missionManager->inProgress()) {
        _multiSpy->waitForSignalByIndex(newMissionItemsAvailableSignalIndex, _signalWaitTime);
        _multiSpy->waitForSignalByIndex(inProgressChangedSignalIndex, _signalWaitTime);
        QCOMPARE(_multiSpy->checkSignalByMask(newMissionItemsAvailableSignalMask | inProgressChangedSignalMask), true);
        QCOMPARE(_multiSpy->checkNoSignalByMask(canEditChangedSignalIndex), true);
    }
    
    QVERIFY(!_missionManager->inProgress());
    QCOMPARE(_missionManager->missionItems()->count(), 0);
    _multiSpy->clearAllSignals();
}

void MissionManagerTest::cleanup(void)
{
    delete _multiSpy;
    _multiSpy = NULL;
    
    LinkManager::instance()->disconnectLink(_mockLink);
    _mockLink = NULL;
    QTest::qWait(1000); // Need to allow signals to move between threads
    
    UnitTest::cleanup();
}

/// Checks the state of the inProgress value and signal to match the specified value
void MissionManagerTest::_checkInProgressValues(bool inProgress)
{
    QCOMPARE(_missionManager->inProgress(), inProgress);
    QSignalSpy* spy = _multiSpy->getSpyByIndex(inProgressChangedSignalIndex);
    QList<QVariant> signalArgs = spy->takeFirst();
    QCOMPARE(signalArgs.count(), 1);
    QCOMPARE(signalArgs[0].toBool(), inProgress);
}

void MissionManagerTest::_readEmptyVehicle(void)
{
    _missionManager->requestMissionItems();

    // requestMissionItems should emit inProgressChanged signal before returning so no need to wait for it
    QVERIFY(_missionManager->inProgress());
    QCOMPARE(_multiSpy->checkOnlySignalByMask(inProgressChangedSignalMask), true);
    _checkInProgressValues(true);
    
    _multiSpy->clearAllSignals();
    
    // Now wait for read sequence to complete. We should get both a newMissionItemsAvailable and a
    // inProgressChanged signal to signal completion.
    _multiSpy->waitForSignalByIndex(newMissionItemsAvailableSignalIndex, _signalWaitTime);
    _multiSpy->waitForSignalByIndex(inProgressChangedSignalIndex, _signalWaitTime);
    QCOMPARE(_multiSpy->checkSignalByMask(newMissionItemsAvailableSignalMask | inProgressChangedSignalMask), true);
    QCOMPARE(_multiSpy->checkNoSignalByMask(canEditChangedSignalMask), true);
    _checkInProgressValues(false);
    
    // Vehicle should have no items at this point
    QCOMPARE(_missionManager->missionItems()->count(), 0);
    QCOMPARE(_missionManager->canEdit(), true);
}

void MissionManagerTest::_writeItems(MockLinkMissionItemHandler::FailureMode_t failureMode, MissionManager::ErrorCode_t errorCode, bool failFirstTimeOnly)
{
    _mockLink->setMissionItemFailureMode(failureMode, failFirstTimeOnly);
    if (failFirstTimeOnly) {
        // Should fail first time, then retry should succed
        failureMode = MockLinkMissionItemHandler::FailNone;
    }
    
    // Setup our test case data
    const size_t cTestCases = sizeof(_rgTestCases)/sizeof(_rgTestCases[0]);
    QmlObjectListModel* list = new QmlObjectListModel();
    
    for (size_t i=0; i<cTestCases; i++) {
        const TestCase_t* testCase = &_rgTestCases[i];
        
        MissionItem* item = new MissionItem(list);
        
        QTextStream loadStream(testCase->itemStream, QIODevice::ReadOnly);
        QVERIFY(item->load(loadStream));
        
        list->append(item);
    }
    
    // Send the items to the vehicle
    _missionManager->writeMissionItems(*list);
    
    // writeMissionItems should emit inProgressChanged signal before returning so no need to wait for it
    QVERIFY(_missionManager->inProgress());
    QCOMPARE(_multiSpy->checkOnlySignalByMask(inProgressChangedSignalMask), true);
    _checkInProgressValues(true);
    
    _multiSpy->clearAllSignals();
    
    if (failureMode == MockLinkMissionItemHandler::FailNone) {
        // This should be clean run
        
        // Wait for write sequence to complete. We should get:
        //      inProgressChanged(false) signal
        _multiSpy->waitForSignalByIndex(inProgressChangedSignalIndex, _signalWaitTime);
        QCOMPARE(_multiSpy->checkOnlySignalByMask(inProgressChangedSignalMask), true);
        
        // Validate inProgressChanged signal value
        _checkInProgressValues(false);

        // We should have gotten back all mission items
        QCOMPARE(_missionManager->missionItems()->count(), (int)cTestCases);
    } else {
        // This should be a failed run
        
        setExpectedMessageBox(QMessageBox::Ok);

        // Wait for write sequence to complete. We should get:
        //      inProgressChanged(false) signal
        //      error(errorCode, QString) signal
        _multiSpy->waitForSignalByIndex(inProgressChangedSignalIndex, _signalWaitTime);
        QCOMPARE(_multiSpy->checkSignalByMask(inProgressChangedSignalMask | errorSignalMask), true);
        
        // Validate inProgressChanged signal value
        _checkInProgressValues(false);
        
        // Validate error signal values
        QSignalSpy* spy = _multiSpy->getSpyByIndex(errorSignalIndex);
        QList<QVariant> signalArgs = spy->takeFirst();
        QCOMPARE(signalArgs.count(), 2);
        qDebug() << signalArgs[1].toString();
        QCOMPARE(signalArgs[0].toInt(), (int)errorCode);

        checkExpectedMessageBox();
    }
    
    QCOMPARE(_missionManager->canEdit(), true);
    
    delete list;
    list = NULL;
    _multiSpy->clearAllSignals();
}

void MissionManagerTest::_roundTripItems(MockLinkMissionItemHandler::FailureMode_t failureMode, MissionManager::ErrorCode_t errorCode, bool failFirstTimeOnly)
{
    _writeItems(MockLinkMissionItemHandler::FailNone, MissionManager::InternalError, false);
    
    _mockLink->setMissionItemFailureMode(failureMode, failFirstTimeOnly);
    if (failFirstTimeOnly) {
        // Should fail first time, then retry should succed
        failureMode = MockLinkMissionItemHandler::FailNone;
    }

    // Read the items back from the vehicle
    _missionManager->requestMissionItems();
    
    // requestMissionItems should emit inProgressChanged signal before returning so no need to wait for it
    QVERIFY(_missionManager->inProgress());
    QCOMPARE(_multiSpy->checkOnlySignalByMask(inProgressChangedSignalMask), true);
    _checkInProgressValues(true);
    
    _multiSpy->clearAllSignals();
    
    if (failureMode == MockLinkMissionItemHandler::FailNone) {
        // This should be clean run
        
        // Now wait for read sequence to complete. We should get:
        //      inProgressChanged(false) signal to signal completion
        //      newMissionItemsAvailable signal
        _multiSpy->waitForSignalByIndex(inProgressChangedSignalIndex, _signalWaitTime);
        QCOMPARE(_multiSpy->checkSignalByMask(newMissionItemsAvailableSignalMask | inProgressChangedSignalMask), true);
        QCOMPARE(_multiSpy->checkNoSignalByMask(canEditChangedSignalMask), true);
        _checkInProgressValues(false);
    } else {
        // This should be a failed run
        
        setExpectedMessageBox(QMessageBox::Ok);

        // Wait for read sequence to complete. We should get:
        //      inProgressChanged(false) signal to signal completion
        //      error(errorCode, QString) signal
        //      newMissionItemsAvailable signal
        _multiSpy->waitForSignalByIndex(inProgressChangedSignalIndex, _signalWaitTime);
        QCOMPARE(_multiSpy->checkSignalByMask(newMissionItemsAvailableSignalMask | inProgressChangedSignalMask | errorSignalMask), true);
        
        // Validate inProgressChanged signal value
        _checkInProgressValues(false);
        
        // Validate error signal values
        QSignalSpy* spy = _multiSpy->getSpyByIndex(errorSignalIndex);
        QList<QVariant> signalArgs = spy->takeFirst();
        QCOMPARE(signalArgs.count(), 2);
        qDebug() << signalArgs[1].toString();
        QCOMPARE(signalArgs[0].toInt(), (int)errorCode);
        
        checkExpectedMessageBox();
    }
    
    _multiSpy->clearAllSignals();

    // Validate returned items
    
    size_t cMissionItemsExpected;
    
    if (failureMode == MockLinkMissionItemHandler::FailNone || failFirstTimeOnly == true) {
        cMissionItemsExpected = sizeof(_rgTestCases)/sizeof(_rgTestCases[0]);
    } else {
        switch (failureMode) {
            case MockLinkMissionItemHandler::FailReadRequestListNoResponse:
            case MockLinkMissionItemHandler::FailReadRequest0NoResponse:
            case MockLinkMissionItemHandler::FailReadRequest0IncorrectSequence:
            case MockLinkMissionItemHandler::FailReadRequest0ErrorAck:
                cMissionItemsExpected = 0;
                break;
            case MockLinkMissionItemHandler::FailReadRequest1NoResponse:
            case MockLinkMissionItemHandler::FailReadRequest1IncorrectSequence:
            case MockLinkMissionItemHandler::FailReadRequest1ErrorAck:
                cMissionItemsExpected = 1;
                break;
            default:
                // Internal error
                Q_ASSERT(false);
                break;
        }
    }
    
    QCOMPARE(_missionManager->missionItems()->count(), (int)cMissionItemsExpected);
    QCOMPARE(_missionManager->canEdit(), true);
    
    for (size_t i=0; i<cMissionItemsExpected; i++) {
        const TestCase_t* testCase = &_rgTestCases[i];
        MissionItem* actual = qobject_cast<MissionItem*>(_missionManager->missionItems()->get(i));
        
        qDebug() << "Test case" << i;
        QCOMPARE(actual->coordinate().latitude(),   testCase->expectedItem.coordinate.latitude());
        QCOMPARE(actual->coordinate().longitude(),  testCase->expectedItem.coordinate.longitude());
        QCOMPARE(actual->coordinate().altitude(),   testCase->expectedItem.coordinate.altitude());
        QCOMPARE((int)actual->command(),       (int)testCase->expectedItem.command);
        QCOMPARE(actual->param1(),                  testCase->expectedItem.param1);
        QCOMPARE(actual->param2(),                  testCase->expectedItem.param2);
        QCOMPARE(actual->param3(),                  testCase->expectedItem.param3);
        QCOMPARE(actual->param4(),                  testCase->expectedItem.param4);
        QCOMPARE(actual->autoContinue(),            testCase->expectedItem.autocontinue);
        QCOMPARE(actual->frame(),                   testCase->expectedItem.frame);
    }
    
}

void MissionManagerTest::_testWriteFailureHandling(void)
{
    /*
    /// Called to send a MISSION_ACK message while the MissionManager is in idle state
    void sendUnexpectedMissionAck(MAV_MISSION_RESULT ackType) { _missionItemHandler.sendUnexpectedMissionAck(ackType); }
    
    /// Called to send a MISSION_ITEM message while the MissionManager is in idle state
    void sendUnexpectedMissionItem(void) { _missionItemHandler.sendUnexpectedMissionItem(); }
    
    /// Called to send a MISSION_REQUEST message while the MissionManager is in idle state
    void sendUnexpectedMissionRequest(void) { _missionItemHandler.sendUnexpectedMissionRequest(); }
    */
    
    typedef struct {
        const char*                                 failureText;
        MockLinkMissionItemHandler::FailureMode_t   failureMode;
        MissionManager::ErrorCode_t                 errorCode;
    } TestCase_t;
    
    static const TestCase_t rgTestCases[] = {
        { "No Failure",                         MockLinkMissionItemHandler::FailNone,                           MissionManager::AckTimeoutError },
        { "FailWriteRequest0NoResponse",        MockLinkMissionItemHandler::FailWriteRequest0NoResponse,        MissionManager::AckTimeoutError },
        { "FailWriteRequest1NoResponse",        MockLinkMissionItemHandler::FailWriteRequest1NoResponse,        MissionManager::AckTimeoutError },
        { "FailWriteRequest0IncorrectSequence", MockLinkMissionItemHandler::FailWriteRequest0IncorrectSequence, MissionManager::ItemMismatchError },
        { "FailWriteRequest1IncorrectSequence", MockLinkMissionItemHandler::FailWriteRequest1IncorrectSequence, MissionManager::ItemMismatchError },
        { "FailWriteRequest0ErrorAck",          MockLinkMissionItemHandler::FailWriteRequest0ErrorAck,          MissionManager::VehicleError },
        { "FailWriteRequest1ErrorAck",          MockLinkMissionItemHandler::FailWriteRequest1ErrorAck,          MissionManager::VehicleError },
        { "FailWriteFinalAckNoResponse",        MockLinkMissionItemHandler::FailWriteFinalAckNoResponse,        MissionManager::AckTimeoutError },
        { "FailWriteFinalAckErrorAck",          MockLinkMissionItemHandler::FailWriteFinalAckErrorAck,          MissionManager::VehicleError },
        { "FailWriteFinalAckMissingRequests",   MockLinkMissionItemHandler::FailWriteFinalAckMissingRequests,   MissionManager::MissingRequestsError },
    };

    for (size_t i=0; i<sizeof(rgTestCases)/sizeof(rgTestCases[0]); i++) {
        qDebug() << "TEST CASE " << rgTestCases[i].failureText << "errorCode:" << rgTestCases[i].errorCode << "failFirstTimeOnly:false";
        _writeItems(rgTestCases[i].failureMode, rgTestCases[i].errorCode, false);
        _mockLink->resetMissionItemHandler();
        qDebug() << "TEST CASE " << rgTestCases[i].failureText << "errorCode:" << rgTestCases[i].errorCode << "failFirstTimeOnly:true";
        _writeItems(rgTestCases[i].failureMode, rgTestCases[i].errorCode, true);
        _mockLink->resetMissionItemHandler();
    }
}

void MissionManagerTest::_testReadFailureHandling(void)
{
    /*
     /// Called to send a MISSION_ACK message while the MissionManager is in idle state
     void sendUnexpectedMissionAck(MAV_MISSION_RESULT ackType) { _missionItemHandler.sendUnexpectedMissionAck(ackType); }
     
     /// Called to send a MISSION_ITEM message while the MissionManager is in idle state
     void sendUnexpectedMissionItem(void) { _missionItemHandler.sendUnexpectedMissionItem(); }
     
     /// Called to send a MISSION_REQUEST message while the MissionManager is in idle state
     void sendUnexpectedMissionRequest(void) { _missionItemHandler.sendUnexpectedMissionRequest(); }
     */
    
    typedef struct {
        const char*                                 failureText;
        MockLinkMissionItemHandler::FailureMode_t   failureMode;
        MissionManager::ErrorCode_t                 errorCode;
    } TestCase_t;
    
    static const TestCase_t rgTestCases[] = {
        { "No Failure",                         MockLinkMissionItemHandler::FailNone,                           MissionManager::AckTimeoutError },
        { "FailReadRequestListNoResponse",      MockLinkMissionItemHandler::FailReadRequestListNoResponse,      MissionManager::AckTimeoutError },
        { "FailReadRequest0NoResponse",         MockLinkMissionItemHandler::FailReadRequest0NoResponse,         MissionManager::AckTimeoutError },
        { "FailReadRequest1NoResponse",         MockLinkMissionItemHandler::FailReadRequest1NoResponse,         MissionManager::AckTimeoutError },
        { "FailReadRequest0IncorrectSequence",  MockLinkMissionItemHandler::FailReadRequest0IncorrectSequence,  MissionManager::ItemMismatchError },
        { "FailReadRequest1IncorrectSequence",  MockLinkMissionItemHandler::FailReadRequest1IncorrectSequence,  MissionManager::ItemMismatchError },
        { "FailReadRequest0ErrorAck",           MockLinkMissionItemHandler::FailReadRequest0ErrorAck,           MissionManager::VehicleError },
        { "FailReadRequest1ErrorAck",           MockLinkMissionItemHandler::FailReadRequest1ErrorAck,           MissionManager::VehicleError },
    };
    
    for (size_t i=0; i<sizeof(rgTestCases)/sizeof(rgTestCases[0]); i++) {
        qDebug() << "TEST CASE " << rgTestCases[i].failureText << "errorCode:" << rgTestCases[i].errorCode << "failFirstTimeOnly:false";
        _roundTripItems(rgTestCases[i].failureMode, rgTestCases[i].errorCode, false);
        _mockLink->resetMissionItemHandler();
        qDebug() << "TEST CASE " << rgTestCases[i].failureText << "errorCode:" << rgTestCases[i].errorCode << "failFirstTimeOnly:true";
        _roundTripItems(rgTestCases[i].failureMode, rgTestCases[i].errorCode, true);
        _mockLink->resetMissionItemHandler();
    }
}
