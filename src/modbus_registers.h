#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t addr;        // decimal AISWEI address (e.g. 31001)
    uint16_t length;      // number of 16-bit registers this entry occupies
    const char* name;     // first line of Description/number code
    const char* type;     // type string (U16, S16, U32, etc.)
    const char* unit;     // unit string or NULL
    float gain;           // gain (multiply raw by gain to get real value)
    const char* access;   // "RO", "RW", or "WO"
} RegisterInfo;

extern const RegisterInfo aiswei_registers[];
extern const size_t aiswei_registers_count;

/**
 * Find index of register info for a given decimal AISWEI address.
 * If addr falls into a multi-register entry (addr .. addr+length-1) that entry is returned.
 * Returns -1 if not found.
 */
int aiswei_find_register_index(uint32_t addr_dec);

// Helper (internal) - you can call directly if needed
uint16_t aiswei_dec2reg(uint32_t addr_dec);

void cleanupModbusTCP();
void parseModbusTCPResponse();

/* Read wrappers (input & holding read requests) */
/* Input registers (chapter 3.3) - prototypes */
bool deviceType(uint8_t slave);
bool modbusAddress(uint8_t slave);
bool serialNumber(uint8_t slave);
bool machineType(uint8_t slave);
bool currentGridCode(uint8_t slave);
bool ratedPower_W(uint8_t slave);
bool softwareVersion(uint8_t slave);
bool safetyVersion(uint8_t slave);
bool manufacturerName(uint8_t slave);
bool brandName(uint8_t slave);

bool gridRatedVoltage_V(uint8_t slave);
bool gridRatedFrequency_Hz(uint8_t slave);
bool eToday_kWh(uint8_t slave);
bool eTotal_kWh(uint8_t slave);
bool hTotal_H(uint8_t slave);
bool deviceState(uint8_t slave);
bool connectTime_s(uint8_t slave);

bool airTemperature_C(uint8_t slave);
bool inverterUphaseTemperature_C(uint8_t slave);
bool inverterVphaseTemperature_C(uint8_t slave);
bool inverterWphaseTemperature_C(uint8_t slave);
bool boostTemperature_C(uint8_t slave);
bool bidirectionalDCDCtemperature_C(uint8_t slave);
bool busVoltage_V(uint8_t slave);

bool pv1Voltage_V(uint8_t slave);
bool pv1Current_A(uint8_t slave);
bool pv2Voltage_V(uint8_t slave);
bool pv2Current_A(uint8_t slave);
bool pv3Voltage_V(uint8_t slave);
bool pv3Current_A(uint8_t slave);
bool pv4Voltage_V(uint8_t slave);
bool pv4Current_A(uint8_t slave);
bool pv5Voltage_V(uint8_t slave);
bool pv5Current_A(uint8_t slave);

bool string1Current_A(uint8_t slave);
bool string2Current_A(uint8_t slave);
bool string3Current_A(uint8_t slave);
bool string4Current_A(uint8_t slave);
bool string5Current_A(uint8_t slave);
bool string6Current_A(uint8_t slave);
bool string7Current_A(uint8_t slave);
bool string8Current_A(uint8_t slave);
bool string9Current_A(uint8_t slave);
bool string10Current_A(uint8_t slave);

bool L1PhaseVoltage_V(uint8_t slave);
bool L1PhaseCurrent_A(uint8_t slave);
bool L2PhaseVoltage_V(uint8_t slave);
bool L2PhaseCurrent_A(uint8_t slave);
bool L3PhaseVoltage_V(uint8_t slave);
bool L3PhaseCurrent_A(uint8_t slave);
bool RSLineVoltage_V(uint8_t slave);
bool RTLineVoltage_V(uint8_t slave);
bool STLineVoltage_V(uint8_t slave);

bool gridFrequency_Hz(uint8_t slave);
bool apparentPower_VA(uint8_t slave);
bool activePower_W(uint8_t slave);
bool reactivePower_Var(uint8_t slave);
bool powerFactor(uint8_t slave);

bool errorCode(uint8_t slave);
bool warningCode(uint8_t slave);

// PV / battery related (selected)
bool pvTotalPower_W(uint8_t slave);
bool pvEToday_kWh(uint8_t slave);
bool pvETotal_kWh(uint8_t slave);
bool batteryCommunicationStatus(uint8_t slave);
bool batteryStatus(uint8_t slave);
bool batteryErrorStatus(uint8_t slave);
bool batteryWarningStatus(uint8_t slave);
bool batteryVoltage_V(uint8_t slave);
bool batteryCurrent_A(uint8_t slave);
bool batteryPower_W(uint8_t slave);
bool batteryTemperature_C(uint8_t slave);
bool batterySOC(uint8_t slave);
bool batterySOH(uint8_t slave);
bool batteryChargingCurrentLimit_A(uint8_t slave);
bool batteryDischargeCurrentLimit_A(uint8_t slave);
bool batteryEChargeToday_kWh(uint8_t slave);
bool batteryEDischargeToday_kWh(uint8_t slave);
bool eConsumptionToday_AC_kWh(uint8_t slave);
bool eGenerationToday_AC_kWh(uint8_t slave);
bool EPSLoadVoltage_V(uint8_t slave);
bool EPSLoadCurrent_A(uint8_t slave);
bool EPSLoadFrequency_Hz(uint8_t slave);
bool EPSLoadActivePower_W(uint8_t slave);
bool EPSLoadReactivePower_Var(uint8_t slave);
bool eConsumptionToday_EPS_kWh(uint8_t slave);
bool eConsumptionTotal_EPS_kWh(uint8_t slave);

/* Holding registers (read wrappers) */
bool remoteSwitchCommand(uint8_t slave);
bool rtc_Year(uint8_t slave);
bool rtc_Month(uint8_t slave);
bool rtc_Day(uint8_t slave);
bool rtc_Hour(uint8_t slave);
bool rtc_Minute(uint8_t slave);
bool rtc_Seconds(uint8_t slave);

bool storageInverterSwitch(uint8_t slave);
bool typeSelectionOfEnergyStorageMachine(uint8_t slave);
bool runMode(uint8_t slave);
bool batteryManufacturer(uint8_t slave);
bool smartMeterStatus(uint8_t slave);
bool smartMeterAdjustmentFlag(uint8_t slave);
bool setTargetPowerValue_W(uint8_t slave);
bool currentPowerValueOfSmartMeter_W(uint8_t slave);
bool antiReverseCurrentFlag(uint8_t slave);
bool batteryWakeUp(uint8_t slave);
bool commboxAndCloudCommunicationStatus(uint8_t slave);
bool chargeDischargeFlagBit(uint8_t slave);
bool chargeAndDischargePowerCommand_W(uint8_t slave);

bool activePowerControlFunction(uint8_t slave);
bool eegControlFunction(uint8_t slave);
bool slopeLoadFunction(uint8_t slave);
bool overvoltageReducePowerFunction(uint8_t slave);
bool overfrequencyReducePowerFunction(uint8_t slave);
bool reactivePowerControlFunction(uint8_t slave);
bool LVRTFunction(uint8_t slave);
bool tenMinutesAverageOvervoltageProtectFunction(uint8_t slave);
bool islandingProtectFunction(uint8_t slave);
bool peConnectionCheckFunction(uint8_t slave);
bool overloadFunction(uint8_t slave);
bool shadowMPPTFunction(uint8_t slave);

/* 45xxx & 454xx & 455xx groups (read) */
bool gridCode(uint8_t slave);
bool overvoltageProtectionValue3_V(uint8_t slave);
bool overvoltageProtectionValue2_V(uint8_t slave);
bool overvoltageProtectionValueFreq_Hz(uint8_t slave);
bool underfrequencyProtectionValue_Hz(uint8_t slave);
bool gridVoltageHighLimit3_V(uint8_t slave);
bool gridVoltageHighLimitTime3_ms(uint8_t slave);
bool gridVoltageHighLimit2_V(uint8_t slave);
bool gridVoltageHighLimitTime2_ms(uint8_t slave);
bool gridVoltageHighLimit1_V(uint8_t slave);
bool gridVoltageHighLimitTime1_ms(uint8_t slave);
bool gridVoltageLowLimit3_V(uint8_t slave);
bool gridVoltageLowLimitTime3_ms(uint8_t slave);
bool gridVoltageLowLimit2_V(uint8_t slave);
bool gridVoltageLowLimitTime2_ms(uint8_t slave);
bool gridVoltageLowLimit1_V(uint8_t slave);
bool gridVoltageLowLimitTime1_ms(uint8_t slave);
bool tenMinutesAverageOvervoltageThreshold_V(uint8_t slave);
bool tenMinutesAverageOvervoltageProtectTime_ms(uint8_t slave);
bool overvoltageRecoverValue_V(uint8_t slave);
bool undervoltageRecoverValue_V(uint8_t slave);
bool gridFrequencyHighLimit3_Hz(uint8_t slave);
bool gridFrequencyHighLimitTime3_ms(uint8_t slave);
bool gridFrequencyHighLimit2_Hz(uint8_t slave);
bool gridFrequencyHighLimitTime2_ms(uint8_t slave);
bool gridFrequencyHighLimit1_Hz(uint8_t slave);
bool gridFrequencyHighLimitTime1_ms(uint8_t slave);
bool gridFrequencyLowLimit3_Hz(uint8_t slave);
bool gridFrequencyLowLimitTime3_ms(uint8_t slave);
bool gridFrequencyLowLimit2_Hz(uint8_t slave);
bool gridFrequencyLowLimitTime2_ms(uint8_t slave);
bool gridFrequencyLowLimit1_Hz(uint8_t slave);
bool gridFrequencyLowLimitTime1_ms(uint8_t slave);
bool varyRateOfFrequencyProtectValue_HzPerS(uint8_t slave);
bool varyRateOfFrequencyProtectTime_ms(uint8_t slave);
bool overfrequencyRecoverValue_Hz(uint8_t slave);
bool underfrequencyRecoverValue_Hz(uint8_t slave);
bool timeOfFirstConnectionToGrid_s(uint8_t slave);
bool timeOfReconnectionToGrid_s(uint8_t slave);
bool isoProtectThreshold_kOhm(uint8_t slave);
bool dciProtectThreshold_mA(uint8_t slave);
bool dciProtectTime_ms(uint8_t slave);

bool loadRateOfFirstConnectionToGrid(uint8_t slave);
bool loadRateOfReconnectionToGrid(uint8_t slave);
bool activePowerSet_percentPn(uint8_t slave);
bool increaseRateOfActivePower_percentPnPerMin(uint8_t slave);
bool decreaseRateOfActivePower_percentPnPerMin(uint8_t slave);
bool overFrequencyReducePowerMode(uint8_t slave);
bool overFrequencyReducePowerStartFrequency_Hz(uint8_t slave);
bool overFrequencyReducePowerStopFrequency_Hz(uint8_t slave);
bool overFrequencyReducePowerBackFrequency_Hz(uint8_t slave);
bool reduceRatioOfOverFrequencyReducePower(uint8_t slave);
bool overFrequencyReducePowerReduceDelayTime_s(uint8_t slave);
bool overFrequencyReducePowerRecoverDelayTime_s(uint8_t slave);
bool speedOfOverFrequencyRecoverToPn(uint8_t slave);
bool overFrequencyZeroPowerFrequencyPoint_Hz(uint8_t slave);

bool overVoltageReducePowerMode(uint8_t slave);
bool overVoltageReducePowerStartVoltage_percentUn(uint8_t slave);
bool overVoltageReducePowerStopVoltage_percentUn(uint8_t slave);
bool overVoltageReducePowerBackVoltage_percentUn(uint8_t slave);
bool reduceRatioOfOverVoltageReducePower(uint8_t slave);
bool overVoltageReducePowerDelayTime_s(uint8_t slave);
bool overVoltageRecoverPowerDelayTime_s(uint8_t slave);
bool speedOfOverVoltageRecoverToPn(uint8_t slave);

bool underFrequencyIncreasePowerMode(uint8_t slave);
bool underFrequencyIncreasePowerStartFrequency_Hz(uint8_t slave);
bool underFrequencyIncreasePowerStopFrequency_Hz(uint8_t slave);
bool underFrequencyIncreasePowerBackFrequency_Hz(uint8_t slave);
bool increaseRatioOfUnderFrequencyIncreasePower(uint8_t slave);
bool underFrequencyIncreasePowerDelayTime_s(uint8_t slave);
bool underFrequencyRecoverPowerDelayTime_s(uint8_t slave);
bool speedOfUnderFrequencyRecoverToPn(uint8_t slave);
bool underFrequencyZeroPowerFrequencyPoint_Hz(uint8_t slave);

bool underVoltageIncreasePowerMode(uint8_t slave);
bool underVoltageIncreasePowerStartVoltage_percentUn(uint8_t slave);
bool underVoltageIncreasePowerStopVoltage_percentUn(uint8_t slave);
bool underVoltageIncreasePowerBackVoltage_percentUn(uint8_t slave);
bool increaseRatioOfUnderVoltageIncreasePower(uint8_t slave);
bool underVoltageIncreasePowerDelayTime_s(uint8_t slave);
bool underVoltageIncreasePowerDelayTime2_s(uint8_t slave);
bool speedOfUnderVoltageRecoverToPn(uint8_t slave);

bool pav_percentPn(uint8_t slave);
bool drmsPval_percentPn(uint8_t slave);

bool reactivePowerControlMode(uint8_t slave);
bool timeConstantOfReactivePowerCurve_s(uint8_t slave);
bool pfSetValue(uint8_t slave); // 45503
bool cosPPowerCurve_point1_activePercentPn(uint8_t slave);
bool cosPPowerCurve_point1_cosPhi(uint8_t slave);
bool cosPPowerCurve_point2_activePercentPn(uint8_t slave);
bool cosPPowerCurve_point2_cosPhi(uint8_t slave);
bool cosPPowerCurve_point3_activePercentPn(uint8_t slave);
bool cosPPowerCurve_point3_cosPhi(uint8_t slave);
bool cosPPowerCurve_point4_activePercentPn(uint8_t slave);
bool cosPPowerCurve_point4_cosPhi(uint8_t slave);
bool lockInVoltageForCosPPowerCurve_percentUn(uint8_t slave);
bool lockOutVoltageForCosPPowerCurve_percentUn(uint8_t slave);
bool QSetValue_percentSn(uint8_t slave);
bool QU_curve_point1_U_percentUn(uint8_t slave);
bool QU_curve_point1_Q_percentSn(uint8_t slave);
bool QU_curve_point2_U_percentUn(uint8_t slave);
bool QU_curve_point2_Q_percentSn(uint8_t slave);
bool QU_curve_point3_U_percentUn(uint8_t slave);
bool QU_curve_point3_Q_percentSn(uint8_t slave);
bool QU_curve_point4_U_percentUn(uint8_t slave);
bool QU_curve_point4_Q_percentSn(uint8_t slave);
bool lockInPowerForQU_curve_percentPn(uint8_t slave);
bool lockOutPowerForQU_curve_percentPn(uint8_t slave);

bool LVRT_TriggerVoltage_percentUn(uint8_t slave);
bool LVRT_activePowerLimitMode(uint8_t slave);

/* Per-register write wrappers for RW holding registers (return true on request issued) */
/* single- and multi-register writers (names correspond to read wrappers with write_ prefix) */
bool write_remoteSwitchCommand(uint8_t slave, uint16_t value);
bool write_rtc_Year(uint8_t slave, uint16_t value);
bool write_rtc_Month(uint8_t slave, uint16_t value);
bool write_rtc_Day(uint8_t slave, uint16_t value);
bool write_rtc_Hour(uint8_t slave, uint16_t value);
bool write_rtc_Minute(uint8_t slave, uint16_t value);
bool write_rtc_Seconds(uint8_t slave, uint16_t value);

bool write_storageInverterSwitch(uint8_t slave, uint16_t value);
bool write_typeSelectionOfEnergyStorageMachine(uint8_t slave, uint16_t value);
bool write_runMode(uint8_t slave, uint16_t value);
bool write_batteryManufacturer(uint8_t slave, uint16_t value);
bool write_smartMeterStatus(uint8_t slave, uint16_t value);
bool write_smartMeterAdjustmentFlag(uint8_t slave, uint16_t value);
bool write_setTargetPowerValue_W(uint8_t slave, uint32_t value);
bool write_currentPowerValueOfSmartMeter_W(uint8_t slave, uint32_t value);
bool write_antiReverseCurrentFlag(uint8_t slave, uint16_t value);
bool write_batteryWakeUp(uint8_t slave, uint16_t value);
bool write_commboxAndCloudCommunicationStatus(uint8_t slave, uint16_t value);
bool write_chargeDischargeFlagBit(uint8_t slave, uint16_t value);
bool write_chargeAndDischargePowerCommand_W(uint8_t slave, uint16_t value);

bool write_activePowerControlFunction(uint8_t slave, uint16_t value);
bool write_eegControlFunction(uint8_t slave, uint16_t value);
bool write_slopeLoadFunction(uint8_t slave, uint16_t value);
bool write_overvoltageReducePowerFunction(uint8_t slave, uint16_t value);
bool write_overfrequencyReducePowerFunction(uint8_t slave, uint16_t value);
bool write_reactivePowerControlFunction(uint8_t slave, uint16_t value);
bool write_LVRTFunction(uint8_t slave, uint16_t value);
bool write_tenMinutesAverageOvervoltageProtectFunction(uint8_t slave, uint16_t value);
bool write_islandingProtectFunction(uint8_t slave, uint16_t value);
bool write_peConnectionCheckFunction(uint8_t slave, uint16_t value);
bool write_overloadFunction(uint8_t slave, uint16_t value);
bool write_shadowMPPTFunction(uint8_t slave, uint16_t value);

bool write_gridCode(uint8_t slave, uint16_t value);
bool write_overvoltageProtectionValue3_V(uint8_t slave, uint16_t value);
bool write_overvoltageProtectionValue2_V(uint8_t slave, uint16_t value);
bool write_overvoltageProtectionValueFreq_Hz(uint8_t slave, uint16_t value);
bool write_underfrequencyProtectionValue_Hz(uint8_t slave, uint16_t value);
bool write_gridVoltageHighLimit3_V(uint8_t slave, uint16_t value);
bool write_gridVoltageHighLimitTime3_ms(uint8_t slave, uint32_t value);
bool write_gridVoltageHighLimit2_V(uint8_t slave, uint16_t value);
bool write_gridVoltageHighLimitTime2_ms(uint8_t slave, uint32_t value);
bool write_gridVoltageHighLimit1_V(uint8_t slave, uint16_t value);
bool write_gridVoltageHighLimitTime1_ms(uint8_t slave, uint32_t value);
bool write_gridVoltageLowLimit3_V(uint8_t slave, uint16_t value);
bool write_gridVoltageLowLimitTime3_ms(uint8_t slave, uint32_t value);
bool write_gridVoltageLowLimit2_V(uint8_t slave, uint16_t value);
bool write_gridVoltageLowLimitTime2_ms(uint8_t slave, uint32_t value);
bool write_gridVoltageLowLimit1_V(uint8_t slave, uint16_t value);
bool write_gridVoltageLowLimitTime1_ms(uint8_t slave, uint32_t value);
bool write_tenMinutesAverageOvervoltageThreshold_V(uint8_t slave, uint16_t value);
bool write_tenMinutesAverageOvervoltageProtectTime_ms(uint8_t slave, uint16_t value);
bool write_overvoltageRecoverValue_V(uint8_t slave, uint16_t value);
bool write_undervoltageRecoverValue_V(uint8_t slave, uint16_t value);
bool write_gridFrequencyHighLimit3_Hz(uint8_t slave, uint16_t value);
bool write_gridFrequencyHighLimitTime3_ms(uint8_t slave, uint32_t value);
bool write_gridFrequencyHighLimit2_Hz(uint8_t slave, uint16_t value);
bool write_gridFrequencyHighLimitTime2_ms(uint8_t slave, uint32_t value);
bool write_gridFrequencyHighLimit1_Hz(uint8_t slave, uint16_t value);
bool write_gridFrequencyHighLimitTime1_ms(uint8_t slave, uint32_t value);
bool write_gridFrequencyLowLimit3_Hz(uint8_t slave, uint16_t value);
bool write_gridFrequencyLowLimitTime3_ms(uint8_t slave, uint32_t value);
bool write_gridFrequencyLowLimit2_Hz(uint8_t slave, uint16_t value);
bool write_gridFrequencyLowLimitTime2_ms(uint8_t slave, uint32_t value);
bool write_gridFrequencyLowLimit1_Hz(uint8_t slave, uint16_t value);
bool write_gridFrequencyLowLimitTime1_ms(uint8_t slave, uint32_t value);
bool write_varyRateOfFrequencyProtectValue_HzPerS(uint8_t slave, uint16_t value);
bool write_varyRateOfFrequencyProtectTime_ms(uint8_t slave, uint32_t value);
bool write_overfrequencyRecoverValue_Hz(uint8_t slave, uint16_t value);
bool write_underfrequencyRecoverValue_Hz(uint8_t slave, uint16_t value);
bool write_timeOfFirstConnectionToGrid_s(uint8_t slave, uint16_t value);
bool write_timeOfReconnectionToGrid_s(uint8_t slave, uint16_t value);
bool write_isoProtectThreshold_kOhm(uint8_t slave, uint16_t value);
bool write_dciProtectThreshold_mA(uint8_t slave, uint16_t value);
bool write_dciProtectTime_ms(uint8_t slave, uint16_t value);

bool write_loadRateOfFirstConnectionToGrid(uint8_t slave, uint16_t value);
bool write_loadRateOfReconnectionToGrid(uint8_t slave, uint16_t value);
bool write_activePowerSet_percentPn(uint8_t slave, uint16_t value);
bool write_increaseRateOfActivePower_percentPnPerMin(uint8_t slave, uint16_t value);
bool write_decreaseRateOfActivePower_percentPnPerMin(uint8_t slave, uint16_t value);
bool write_overFrequencyReducePowerMode(uint8_t slave, uint16_t value);
bool write_overFrequencyReducePowerStartFrequency_Hz(uint8_t slave, uint16_t value);
bool write_overFrequencyReducePowerStopFrequency_Hz(uint8_t slave, uint16_t value);
bool write_overFrequencyReducePowerBackFrequency_Hz(uint8_t slave, uint16_t value);
bool write_reduceRatioOfOverFrequencyReducePower(uint8_t slave, uint16_t value);
bool write_overFrequencyReducePowerReduceDelayTime_s(uint8_t slave, uint16_t value);
bool write_overFrequencyReducePowerRecoverDelayTime_s(uint8_t slave, uint16_t value);
bool write_speedOfOverFrequencyRecoverToPn(uint8_t slave, uint16_t value);
bool write_overFrequencyZeroPowerFrequencyPoint_Hz(uint8_t slave, uint16_t value);

bool write_overVoltageReducePowerMode(uint8_t slave, uint16_t value);
bool write_overVoltageReducePowerStartVoltage_percentUn(uint8_t slave, uint16_t value);
bool write_overVoltageReducePowerStopVoltage_percentUn(uint8_t slave, uint16_t value);
bool write_overVoltageReducePowerBackVoltage_percentUn(uint8_t slave, uint16_t value);
bool write_reduceRatioOfOverVoltageReducePower(uint8_t slave, uint16_t value);
bool write_overVoltageReducePowerDelayTime_s(uint8_t slave, uint16_t value);
bool write_overVoltageRecoverPowerDelayTime_s(uint8_t slave, uint16_t value);
bool write_speedOfOverVoltageRecoverToPn(uint8_t slave, uint16_t value);

bool write_underFrequencyIncreasePowerMode(uint8_t slave, uint16_t value);
bool write_underFrequencyIncreasePowerStartFrequency_Hz(uint8_t slave, uint16_t value);
bool write_underFrequencyIncreasePowerStopFrequency_Hz(uint8_t slave, uint16_t value);
bool write_underFrequencyIncreasePowerBackFrequency_Hz(uint8_t slave, uint16_t value);
bool write_increaseRatioOfUnderFrequencyIncreasePower(uint8_t slave, uint16_t value);
bool write_underFrequencyIncreasePowerDelayTime_s(uint8_t slave, uint16_t value);
bool write_underFrequencyRecoverPowerDelayTime_s(uint8_t slave, uint16_t value);
bool write_speedOfUnderFrequencyRecoverToPn(uint8_t slave, uint16_t value);
bool write_underFrequencyZeroPowerFrequencyPoint_Hz(uint8_t slave, uint16_t value);

bool write_underVoltageIncreasePowerMode(uint8_t slave, uint16_t value);
bool write_underVoltageIncreasePowerStartVoltage_percentUn(uint8_t slave, uint16_t value);
bool write_underVoltageIncreasePowerStopVoltage_percentUn(uint8_t slave, uint16_t value);
bool write_underVoltageIncreasePowerBackVoltage_percentUn(uint8_t slave, uint16_t value);
bool write_increaseRatioOfUnderVoltageIncreasePower(uint8_t slave, uint16_t value);
bool write_underVoltageIncreasePowerDelayTime_s(uint8_t slave, uint16_t value);
bool write_underVoltageIncreasePowerDelayTime2_s(uint8_t slave, uint16_t value);
bool write_speedOfUnderVoltageRecoverToPn(uint8_t slave, uint16_t value);

bool write_pav_percentPn(uint8_t slave, uint16_t value);
bool write_drmsPval_percentPn(uint8_t slave, uint16_t value);

bool write_reactivePowerControlMode(uint8_t slave, uint16_t value);
bool write_timeConstantOfReactivePowerCurve_s(uint8_t slave, uint16_t value);
bool write_pfSetValue(uint8_t slave, uint16_t value);
bool write_cosPPowerCurve_point1_activePercentPn(uint8_t slave, uint16_t value);
bool write_cosPPowerCurve_point1_cosPhi(uint8_t slave, uint16_t value);
bool write_cosPPowerCurve_point2_activePercentPn(uint8_t slave, uint16_t value);
bool write_cosPPowerCurve_point2_cosPhi(uint8_t slave, uint16_t value);
bool write_cosPPowerCurve_point3_activePercentPn(uint8_t slave, uint16_t value);
bool write_cosPPowerCurve_point3_cosPhi(uint8_t slave, uint16_t value);
bool write_cosPPowerCurve_point4_activePercentPn(uint8_t slave, uint16_t value);
bool write_cosPPowerCurve_point4_cosPhi(uint8_t slave, uint16_t value);
bool write_lockInVoltageForCosPPowerCurve_percentUn(uint8_t slave, uint16_t value);
bool write_lockOutVoltageForCosPPowerCurve_percentUn(uint8_t slave, uint16_t value);
bool write_QSetValue_percentSn(uint8_t slave, uint16_t value);
bool write_QU_curve_point1_U_percentUn(uint8_t slave, uint16_t value);
bool write_QU_curve_point1_Q_percentSn(uint8_t slave, uint16_t value);
bool write_QU_curve_point2_U_percentUn(uint8_t slave, uint16_t value);
bool write_QU_curve_point2_Q_percentSn(uint8_t slave, uint16_t value);
bool write_QU_curve_point3_U_percentUn(uint8_t slave, uint16_t value);
bool write_QU_curve_point3_Q_percentSn(uint8_t slave, uint16_t value);
bool write_QU_curve_point4_U_percentUn(uint8_t slave, uint16_t value);
bool write_QU_curve_point4_Q_percentSn(uint8_t slave, uint16_t value);
bool write_lockInPowerForQU_curve_percentPn(uint8_t slave, uint16_t value);
bool write_lockOutPowerForQU_curve_percentPn(uint8_t slave, uint16_t value);

bool write_LVRT_TriggerVoltage_percentUn(uint8_t slave, uint16_t value);
bool write_LVRT_activePowerLimitMode(uint8_t slave, uint16_t value);

#ifdef __cplusplus
}
#endif