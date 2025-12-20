#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t addr;        // decimal AISWEI address (e.g. 31001)
    uint16_t length;      // number of 16-bit registers this entry occupies
    const char* name;     // first line of Description/number code
    const char* type;     // type string (U16, S16, U32, etc.)
    const char* unit;     // unit string or NULL
    float gain;           // gain (multiply raw by gain to get real value)
    const char* access;   // "RO", "RW", or "WO"
} RegisterInfo;

extern RegisterInfo aiswei_registers[];
extern const size_t aiswei_registers_count;

/**
 * Find index of register info for a given decimal AISWEI address.
 * If addr falls into a multi-register entry (addr .. addr+length-1) that entry is returned.
 * Returns -1 if not found.
 */
int aiswei_find_register_index(uint16_t addr_dec);

// Helper (internal) - you can call directly if needed
uint16_t aiswei_dec2reg(uint16_t addr_dec);

void cleanupModbusTCP();
bool parseModbusTCPResponse();

bool requestAisweiRead(uint8_t unitId, uint16_t addr_dec);
// Read a contiguous range of AISWEI registers starting at decimal address
// `start_addr_dec` for `quantity` 16-bit registers. This will set the
// internal transaction address so the response parser can dispatch per-register
// decoding. Returns true if request was issued.
bool requestAisweiReadRange(uint8_t unitId, uint16_t start_addr_dec, uint16_t quantity);
bool requestAisweiWriteWord(uint8_t unitId, uint16_t addr_dec, uint16_t value);
bool requestAisweiWriteDWord(uint8_t unitId, uint16_t addr_dec, uint32_t value);

/* Read wrappers (input & holding read requests) */
/* Input registers (chapter 3.3) - prototypes */
bool deviceType(uint8_t unitId);
bool modbusAddress(uint8_t unitId);
bool serialNumber(uint8_t unitId);
bool machineType(uint8_t unitId);
bool currentGridCode(uint8_t unitId);
bool ratedPower_W(uint8_t unitId);
bool softwareVersion(uint8_t unitId);
bool safetyVersion(uint8_t unitId);
bool manufacturerName(uint8_t unitId);
bool brandName(uint8_t unitId);

bool gridRatedVoltage_V(uint8_t unitId);
bool gridRatedFrequency_Hz(uint8_t unitId);
bool eToday_kWh(uint8_t unitId);
bool eTotal_kWh(uint8_t unitId);
bool hTotal_H(uint8_t unitId);
bool deviceState(uint8_t unitId);
bool connectTime_s(uint8_t unitId);

bool airTemperature_C(uint8_t unitId);
bool inverterUphaseTemperature_C(uint8_t unitId);
bool inverterVphaseTemperature_C(uint8_t unitId);
bool inverterWphaseTemperature_C(uint8_t unitId);
bool boostTemperature_C(uint8_t unitId);
bool bidirectionalDCDCtemperature_C(uint8_t unitId);
bool busVoltage_V(uint8_t unitId);

bool pv1Voltage_V(uint8_t unitId);
bool pv1Current_A(uint8_t unitId);
bool pv2Voltage_V(uint8_t unitId);
bool pv2Current_A(uint8_t unitId);
bool pv3Voltage_V(uint8_t unitId);
bool pv3Current_A(uint8_t unitId);
bool pv4Voltage_V(uint8_t unitId);
bool pv4Current_A(uint8_t unitId);
bool pv5Voltage_V(uint8_t unitId);
bool pv5Current_A(uint8_t unitId);

bool string1Current_A(uint8_t unitId);
bool string2Current_A(uint8_t unitId);
bool string3Current_A(uint8_t unitId);
bool string4Current_A(uint8_t unitId);
bool string5Current_A(uint8_t unitId);
bool string6Current_A(uint8_t unitId);
bool string7Current_A(uint8_t unitId);
bool string8Current_A(uint8_t unitId);
bool string9Current_A(uint8_t unitId);
bool string10Current_A(uint8_t unitId);

bool L1PhaseVoltage_V(uint8_t unitId);
bool L1PhaseCurrent_A(uint8_t unitId);
bool L2PhaseVoltage_V(uint8_t unitId);
bool L2PhaseCurrent_A(uint8_t unitId);
bool L3PhaseVoltage_V(uint8_t unitId);
bool L3PhaseCurrent_A(uint8_t unitId);
bool RSLineVoltage_V(uint8_t unitId);
bool RTLineVoltage_V(uint8_t unitId);
bool STLineVoltage_V(uint8_t unitId);

bool gridFrequency_Hz(uint8_t unitId);
bool apparentPower_VA(uint8_t unitId);
bool activePower_W(uint8_t unitId);
bool reactivePower_Var(uint8_t unitId);
bool powerFactor(uint8_t unitId);

bool errorCode(uint8_t unitId);
bool warningCode(uint8_t unitId);

// PV / battery related (selected)
bool pvTotalPower_W(uint8_t unitId);
bool pvEToday_kWh(uint8_t unitId);
bool pvETotal_kWh(uint8_t unitId);
bool batteryCommunicationStatus(uint8_t unitId);
bool batteryStatus(uint8_t unitId);
bool batteryErrorStatus(uint8_t unitId);
bool batteryWarningStatus(uint8_t unitId);
bool batteryVoltage_V(uint8_t unitId);
bool batteryCurrent_A(uint8_t unitId);
bool batteryPower_W(uint8_t unitId);
bool batteryTemperature_C(uint8_t unitId);
bool batterySOC(uint8_t unitId);
bool batterySOH(uint8_t unitId);
bool batteryChargingCurrentLimit_A(uint8_t unitId);
bool batteryDischargeCurrentLimit_A(uint8_t unitId);
bool batteryEChargeToday_kWh(uint8_t unitId);
bool batteryEDischargeToday_kWh(uint8_t unitId);
bool eConsumptionToday_AC_kWh(uint8_t unitId);
bool eGenerationToday_AC_kWh(uint8_t unitId);
bool EPSLoadVoltage_V(uint8_t unitId);
bool EPSLoadCurrent_A(uint8_t unitId);
bool EPSLoadFrequency_Hz(uint8_t unitId);
bool EPSLoadActivePower_W(uint8_t unitId);
bool EPSLoadReactivePower_Var(uint8_t unitId);
bool eConsumptionToday_EPS_kWh(uint8_t unitId);
bool eConsumptionTotal_EPS_kWh(uint8_t unitId);

/* Holding registers (read wrappers) */
bool remoteSwitchCommand(uint8_t unitId);
bool rtc_Year(uint8_t unitId);
bool rtc_Month(uint8_t unitId);
bool rtc_Day(uint8_t unitId);
bool rtc_Hour(uint8_t unitId);
bool rtc_Minute(uint8_t unitId);
bool rtc_Seconds(uint8_t unitId);

bool storageInverterSwitch(uint8_t unitId);
bool typeSelectionOfEnergyStorageMachine(uint8_t unitId);
bool runMode(uint8_t unitId);
bool batteryManufacturer(uint8_t unitId);
bool smartMeterStatus(uint8_t unitId);
bool smartMeterAdjustmentFlag(uint8_t unitId);
bool setTargetPowerValue_W(uint8_t unitId);
bool currentPowerValueOfSmartMeter_W(uint8_t unitId);
bool antiReverseCurrentFlag(uint8_t unitId);
bool batteryWakeUp(uint8_t unitId);
bool commboxAndCloudCommunicationStatus(uint8_t unitId);
bool chargeDischargeFlagBit(uint8_t unitId);
bool chargeAndDischargePowerCommand_W(uint8_t unitId);

bool activePowerControlFunction(uint8_t unitId);
bool eegControlFunction(uint8_t unitId);
bool slopeLoadFunction(uint8_t unitId);
bool overvoltageReducePowerFunction(uint8_t unitId);
bool overfrequencyReducePowerFunction(uint8_t unitId);
bool reactivePowerControlFunction(uint8_t unitId);
bool LVRTFunction(uint8_t unitId);
bool tenMinutesAverageOvervoltageProtectFunction(uint8_t unitId);
bool islandingProtectFunction(uint8_t unitId);
bool peConnectionCheckFunction(uint8_t unitId);
bool overloadFunction(uint8_t unitId);
bool shadowMPPTFunction(uint8_t unitId);

/* 45xxx & 454xx & 455xx groups (read) */
bool gridCode(uint8_t unitId);
bool overvoltageProtectionValue3_V(uint8_t unitId);
bool overvoltageProtectionValue2_V(uint8_t unitId);
bool overvoltageProtectionValueFreq_Hz(uint8_t unitId);
bool underfrequencyProtectionValue_Hz(uint8_t unitId);
bool gridVoltageHighLimit3_V(uint8_t unitId);
bool gridVoltageHighLimitTime3_ms(uint8_t unitId);
bool gridVoltageHighLimit2_V(uint8_t unitId);
bool gridVoltageHighLimitTime2_ms(uint8_t unitId);
bool gridVoltageHighLimit1_V(uint8_t unitId);
bool gridVoltageHighLimitTime1_ms(uint8_t unitId);
bool gridVoltageLowLimit3_V(uint8_t unitId);
bool gridVoltageLowLimitTime3_ms(uint8_t unitId);
bool gridVoltageLowLimit2_V(uint8_t unitId);
bool gridVoltageLowLimitTime2_ms(uint8_t unitId);
bool gridVoltageLowLimit1_V(uint8_t unitId);
bool gridVoltageLowLimitTime1_ms(uint8_t unitId);
bool tenMinutesAverageOvervoltageThreshold_V(uint8_t unitId);
bool tenMinutesAverageOvervoltageProtectTime_ms(uint8_t unitId);
bool overvoltageRecoverValue_V(uint8_t unitId);
bool undervoltageRecoverValue_V(uint8_t unitId);
bool gridFrequencyHighLimit3_Hz(uint8_t unitId);
bool gridFrequencyHighLimitTime3_ms(uint8_t unitId);
bool gridFrequencyHighLimit2_Hz(uint8_t unitId);
bool gridFrequencyHighLimitTime2_ms(uint8_t unitId);
bool gridFrequencyHighLimit1_Hz(uint8_t unitId);
bool gridFrequencyHighLimitTime1_ms(uint8_t unitId);
bool gridFrequencyLowLimit3_Hz(uint8_t unitId);
bool gridFrequencyLowLimitTime3_ms(uint8_t unitId);
bool gridFrequencyLowLimit2_Hz(uint8_t unitId);
bool gridFrequencyLowLimitTime2_ms(uint8_t unitId);
bool gridFrequencyLowLimit1_Hz(uint8_t unitId);
bool gridFrequencyLowLimitTime1_ms(uint8_t unitId);
bool varyRateOfFrequencyProtectValue_HzPerS(uint8_t unitId);
bool varyRateOfFrequencyProtectTime_ms(uint8_t unitId);
bool overfrequencyRecoverValue_Hz(uint8_t unitId);
bool underfrequencyRecoverValue_Hz(uint8_t unitId);
bool timeOfFirstConnectionToGrid_s(uint8_t unitId);
bool timeOfReconnectionToGrid_s(uint8_t unitId);
bool isoProtectThreshold_kOhm(uint8_t unitId);
bool dciProtectThreshold_mA(uint8_t unitId);
bool dciProtectTime_ms(uint8_t unitId);

bool loadRateOfFirstConnectionToGrid(uint8_t unitId);
bool loadRateOfReconnectionToGrid(uint8_t unitId);
bool activePowerSet_percentPn(uint8_t unitId);
bool increaseRateOfActivePower_percentPnPerMin(uint8_t unitId);
bool decreaseRateOfActivePower_percentPnPerMin(uint8_t unitId);
bool overFrequencyReducePowerMode(uint8_t unitId);
bool overFrequencyReducePowerStartFrequency_Hz(uint8_t unitId);
bool overFrequencyReducePowerStopFrequency_Hz(uint8_t unitId);
bool overFrequencyReducePowerBackFrequency_Hz(uint8_t unitId);
bool reduceRatioOfOverFrequencyReducePower(uint8_t unitId);
bool overFrequencyReducePowerReduceDelayTime_s(uint8_t unitId);
bool overFrequencyReducePowerRecoverDelayTime_s(uint8_t unitId);
bool speedOfOverFrequencyRecoverToPn(uint8_t unitId);
bool overFrequencyZeroPowerFrequencyPoint_Hz(uint8_t unitId);

bool overVoltageReducePowerMode(uint8_t unitId);
bool overVoltageReducePowerStartVoltage_percentUn(uint8_t unitId);
bool overVoltageReducePowerStopVoltage_percentUn(uint8_t unitId);
bool overVoltageReducePowerBackVoltage_percentUn(uint8_t unitId);
bool reduceRatioOfOverVoltageReducePower(uint8_t unitId);
bool overVoltageReducePowerDelayTime_s(uint8_t unitId);
bool overVoltageRecoverPowerDelayTime_s(uint8_t unitId);
bool speedOfOverVoltageRecoverToPn(uint8_t unitId);

bool underFrequencyIncreasePowerMode(uint8_t unitId);
bool underFrequencyIncreasePowerStartFrequency_Hz(uint8_t unitId);
bool underFrequencyIncreasePowerStopFrequency_Hz(uint8_t unitId);
bool underFrequencyIncreasePowerBackFrequency_Hz(uint8_t unitId);
bool increaseRatioOfUnderFrequencyIncreasePower(uint8_t unitId);
bool underFrequencyIncreasePowerDelayTime_s(uint8_t unitId);
bool underFrequencyRecoverPowerDelayTime_s(uint8_t unitId);
bool speedOfUnderFrequencyRecoverToPn(uint8_t unitId);
bool underFrequencyZeroPowerFrequencyPoint_Hz(uint8_t unitId);

bool underVoltageIncreasePowerMode(uint8_t unitId);
bool underVoltageIncreasePowerStartVoltage_percentUn(uint8_t unitId);
bool underVoltageIncreasePowerStopVoltage_percentUn(uint8_t unitId);
bool underVoltageIncreasePowerBackVoltage_percentUn(uint8_t unitId);
bool increaseRatioOfUnderVoltageIncreasePower(uint8_t unitId);
bool underVoltageIncreasePowerDelayTime_s(uint8_t unitId);
bool underVoltageIncreasePowerDelayTime2_s(uint8_t unitId);
bool speedOfUnderVoltageRecoverToPn(uint8_t unitId);

bool pav_percentPn(uint8_t unitId);
bool drmsPval_percentPn(uint8_t unitId);

bool reactivePowerControlMode(uint8_t unitId);
bool timeConstantOfReactivePowerCurve_s(uint8_t unitId);
bool pfSetValue(uint8_t unitId); // 45503
bool cosPPowerCurve_point1_activePercentPn(uint8_t unitId);
bool cosPPowerCurve_point1_cosPhi(uint8_t unitId);
bool cosPPowerCurve_point2_activePercentPn(uint8_t unitId);
bool cosPPowerCurve_point2_cosPhi(uint8_t unitId);
bool cosPPowerCurve_point3_activePercentPn(uint8_t unitId);
bool cosPPowerCurve_point3_cosPhi(uint8_t unitId);
bool cosPPowerCurve_point4_activePercentPn(uint8_t unitId);
bool cosPPowerCurve_point4_cosPhi(uint8_t unitId);
bool lockInVoltageForCosPPowerCurve_percentUn(uint8_t unitId);
bool lockOutVoltageForCosPPowerCurve_percentUn(uint8_t unitId);
bool QSetValue_percentSn(uint8_t unitId);
bool QU_curve_point1_U_percentUn(uint8_t unitId);
bool QU_curve_point1_Q_percentSn(uint8_t unitId);
bool QU_curve_point2_U_percentUn(uint8_t unitId);
bool QU_curve_point2_Q_percentSn(uint8_t unitId);
bool QU_curve_point3_U_percentUn(uint8_t unitId);
bool QU_curve_point3_Q_percentSn(uint8_t unitId);
bool QU_curve_point4_U_percentUn(uint8_t unitId);
bool QU_curve_point4_Q_percentSn(uint8_t unitId);
bool lockInPowerForQU_curve_percentPn(uint8_t unitId);
bool lockOutPowerForQU_curve_percentPn(uint8_t unitId);

bool LVRT_TriggerVoltage_percentUn(uint8_t unitId);
bool LVRT_activePowerLimitMode(uint8_t unitId);

/* Per-register write wrappers for RW holding registers (return true on request issued) */
/* single- and multi-register writers (names correspond to read wrappers with write_ prefix) */
bool write_remoteSwitchCommand(uint8_t unitId, uint16_t value);
bool write_rtc_Year(uint8_t unitId, uint16_t value);
bool write_rtc_Month(uint8_t unitId, uint16_t value);
bool write_rtc_Day(uint8_t unitId, uint16_t value);
bool write_rtc_Hour(uint8_t unitId, uint16_t value);
bool write_rtc_Minute(uint8_t unitId, uint16_t value);
bool write_rtc_Seconds(uint8_t unitId, uint16_t value);

bool write_storageInverterSwitch(uint8_t unitId, uint16_t value);
bool write_typeSelectionOfEnergyStorageMachine(uint8_t unitId, uint16_t value);
bool write_runMode(uint8_t unitId, uint16_t value);
bool write_batteryManufacturer(uint8_t unitId, uint16_t value);
bool write_smartMeterStatus(uint8_t unitId, uint16_t value);
bool write_smartMeterAdjustmentFlag(uint8_t unitId, uint16_t value);
bool write_setTargetPowerValue_W(uint8_t unitId, uint32_t value);
bool write_currentPowerValueOfSmartMeter_W(uint8_t unitId, uint32_t value);
bool write_antiReverseCurrentFlag(uint8_t unitId, uint16_t value);
bool write_batteryWakeUp(uint8_t unitId, uint16_t value);
bool write_commboxAndCloudCommunicationStatus(uint8_t unitId, uint16_t value);
bool write_chargeDischargeFlagBit(uint8_t unitId, uint16_t value);
bool write_chargeAndDischargePowerCommand_W(uint8_t unitId, uint16_t value);

bool write_activePowerControlFunction(uint8_t unitId, uint16_t value);
bool write_eegControlFunction(uint8_t unitId, uint16_t value);
bool write_slopeLoadFunction(uint8_t unitId, uint16_t value);
bool write_overvoltageReducePowerFunction(uint8_t unitId, uint16_t value);
bool write_overfrequencyReducePowerFunction(uint8_t unitId, uint16_t value);
bool write_reactivePowerControlFunction(uint8_t unitId, uint16_t value);
bool write_LVRTFunction(uint8_t unitId, uint16_t value);
bool write_tenMinutesAverageOvervoltageProtectFunction(uint8_t unitId, uint16_t value);
bool write_islandingProtectFunction(uint8_t unitId, uint16_t value);
bool write_peConnectionCheckFunction(uint8_t unitId, uint16_t value);
bool write_overloadFunction(uint8_t unitId, uint16_t value);
bool write_shadowMPPTFunction(uint8_t unitId, uint16_t value);

bool write_gridCode(uint8_t unitId, uint16_t value);
bool write_overvoltageProtectionValue3_V(uint8_t unitId, uint16_t value);
bool write_overvoltageProtectionValue2_V(uint8_t unitId, uint16_t value);
bool write_overvoltageProtectionValueFreq_Hz(uint8_t unitId, uint16_t value);
bool write_underfrequencyProtectionValue_Hz(uint8_t unitId, uint16_t value);
bool write_gridVoltageHighLimit3_V(uint8_t unitId, uint16_t value);
bool write_gridVoltageHighLimitTime3_ms(uint8_t unitId, uint32_t value);
bool write_gridVoltageHighLimit2_V(uint8_t unitId, uint16_t value);
bool write_gridVoltageHighLimitTime2_ms(uint8_t unitId, uint32_t value);
bool write_gridVoltageHighLimit1_V(uint8_t unitId, uint16_t value);
bool write_gridVoltageHighLimitTime1_ms(uint8_t unitId, uint32_t value);
bool write_gridVoltageLowLimit3_V(uint8_t unitId, uint16_t value);
bool write_gridVoltageLowLimitTime3_ms(uint8_t unitId, uint32_t value);
bool write_gridVoltageLowLimit2_V(uint8_t unitId, uint16_t value);
bool write_gridVoltageLowLimitTime2_ms(uint8_t unitId, uint32_t value);
bool write_gridVoltageLowLimit1_V(uint8_t unitId, uint16_t value);
bool write_gridVoltageLowLimitTime1_ms(uint8_t unitId, uint32_t value);
bool write_tenMinutesAverageOvervoltageThreshold_V(uint8_t unitId, uint16_t value);
bool write_tenMinutesAverageOvervoltageProtectTime_ms(uint8_t unitId, uint16_t value);
bool write_overvoltageRecoverValue_V(uint8_t unitId, uint16_t value);
bool write_undervoltageRecoverValue_V(uint8_t unitId, uint16_t value);
bool write_gridFrequencyHighLimit3_Hz(uint8_t unitId, uint16_t value);
bool write_gridFrequencyHighLimitTime3_ms(uint8_t unitId, uint32_t value);
bool write_gridFrequencyHighLimit2_Hz(uint8_t unitId, uint16_t value);
bool write_gridFrequencyHighLimitTime2_ms(uint8_t unitId, uint32_t value);
bool write_gridFrequencyHighLimit1_Hz(uint8_t unitId, uint16_t value);
bool write_gridFrequencyHighLimitTime1_ms(uint8_t unitId, uint32_t value);
bool write_gridFrequencyLowLimit3_Hz(uint8_t unitId, uint16_t value);
bool write_gridFrequencyLowLimitTime3_ms(uint8_t unitId, uint32_t value);
bool write_gridFrequencyLowLimit2_Hz(uint8_t unitId, uint16_t value);
bool write_gridFrequencyLowLimitTime2_ms(uint8_t unitId, uint32_t value);
bool write_gridFrequencyLowLimit1_Hz(uint8_t unitId, uint16_t value);
bool write_gridFrequencyLowLimitTime1_ms(uint8_t unitId, uint32_t value);
bool write_varyRateOfFrequencyProtectValue_HzPerS(uint8_t unitId, uint16_t value);
bool write_varyRateOfFrequencyProtectTime_ms(uint8_t unitId, uint32_t value);
bool write_overfrequencyRecoverValue_Hz(uint8_t unitId, uint16_t value);
bool write_underfrequencyRecoverValue_Hz(uint8_t unitId, uint16_t value);
bool write_timeOfFirstConnectionToGrid_s(uint8_t unitId, uint16_t value);
bool write_timeOfReconnectionToGrid_s(uint8_t unitId, uint16_t value);
bool write_isoProtectThreshold_kOhm(uint8_t unitId, uint16_t value);
bool write_dciProtectThreshold_mA(uint8_t unitId, uint16_t value);
bool write_dciProtectTime_ms(uint8_t unitId, uint16_t value);

bool write_loadRateOfFirstConnectionToGrid(uint8_t unitId, uint16_t value);
bool write_loadRateOfReconnectionToGrid(uint8_t unitId, uint16_t value);
bool write_activePowerSet_percentPn(uint8_t unitId, uint16_t value);
bool write_increaseRateOfActivePower_percentPnPerMin(uint8_t unitId, uint16_t value);
bool write_decreaseRateOfActivePower_percentPnPerMin(uint8_t unitId, uint16_t value);
bool write_overFrequencyReducePowerMode(uint8_t unitId, uint16_t value);
bool write_overFrequencyReducePowerStartFrequency_Hz(uint8_t unitId, uint16_t value);
bool write_overFrequencyReducePowerStopFrequency_Hz(uint8_t unitId, uint16_t value);
bool write_overFrequencyReducePowerBackFrequency_Hz(uint8_t unitId, uint16_t value);
bool write_reduceRatioOfOverFrequencyReducePower(uint8_t unitId, uint16_t value);
bool write_overFrequencyReducePowerReduceDelayTime_s(uint8_t unitId, uint16_t value);
bool write_overFrequencyReducePowerRecoverDelayTime_s(uint8_t unitId, uint16_t value);
bool write_speedOfOverFrequencyRecoverToPn(uint8_t unitId, uint16_t value);
bool write_overFrequencyZeroPowerFrequencyPoint_Hz(uint8_t unitId, uint16_t value);

bool write_overVoltageReducePowerMode(uint8_t unitId, uint16_t value);
bool write_overVoltageReducePowerStartVoltage_percentUn(uint8_t unitId, uint16_t value);
bool write_overVoltageReducePowerStopVoltage_percentUn(uint8_t unitId, uint16_t value);
bool write_overVoltageReducePowerBackVoltage_percentUn(uint8_t unitId, uint16_t value);
bool write_reduceRatioOfOverVoltageReducePower(uint8_t unitId, uint16_t value);
bool write_overVoltageReducePowerDelayTime_s(uint8_t unitId, uint16_t value);
bool write_overVoltageRecoverPowerDelayTime_s(uint8_t unitId, uint16_t value);
bool write_speedOfOverVoltageRecoverToPn(uint8_t unitId, uint16_t value);

bool write_underFrequencyIncreasePowerMode(uint8_t unitId, uint16_t value);
bool write_underFrequencyIncreasePowerStartFrequency_Hz(uint8_t unitId, uint16_t value);
bool write_underFrequencyIncreasePowerStopFrequency_Hz(uint8_t unitId, uint16_t value);
bool write_underFrequencyIncreasePowerBackFrequency_Hz(uint8_t unitId, uint16_t value);
bool write_increaseRatioOfUnderFrequencyIncreasePower(uint8_t unitId, uint16_t value);
bool write_underFrequencyIncreasePowerDelayTime_s(uint8_t unitId, uint16_t value);
bool write_underFrequencyRecoverPowerDelayTime_s(uint8_t unitId, uint16_t value);
bool write_speedOfUnderFrequencyRecoverToPn(uint8_t unitId, uint16_t value);
bool write_underFrequencyZeroPowerFrequencyPoint_Hz(uint8_t unitId, uint16_t value);

bool write_underVoltageIncreasePowerMode(uint8_t unitId, uint16_t value);
bool write_underVoltageIncreasePowerStartVoltage_percentUn(uint8_t unitId, uint16_t value);
bool write_underVoltageIncreasePowerStopVoltage_percentUn(uint8_t unitId, uint16_t value);
bool write_underVoltageIncreasePowerBackVoltage_percentUn(uint8_t unitId, uint16_t value);
bool write_increaseRatioOfUnderVoltageIncreasePower(uint8_t unitId, uint16_t value);
bool write_underVoltageIncreasePowerDelayTime_s(uint8_t unitId, uint16_t value);
bool write_underVoltageIncreasePowerDelayTime2_s(uint8_t unitId, uint16_t value);
bool write_speedOfUnderVoltageRecoverToPn(uint8_t unitId, uint16_t value);

bool write_pav_percentPn(uint8_t unitId, uint16_t value);
bool write_drmsPval_percentPn(uint8_t unitId, uint16_t value);

bool write_reactivePowerControlMode(uint8_t unitId, uint16_t value);
bool write_timeConstantOfReactivePowerCurve_s(uint8_t unitId, uint16_t value);
bool write_pfSetValue(uint8_t unitId, uint16_t value);
bool write_cosPPowerCurve_point1_activePercentPn(uint8_t unitId, uint16_t value);
bool write_cosPPowerCurve_point1_cosPhi(uint8_t unitId, uint16_t value);
bool write_cosPPowerCurve_point2_activePercentPn(uint8_t unitId, uint16_t value);
bool write_cosPPowerCurve_point2_cosPhi(uint8_t unitId, uint16_t value);
bool write_cosPPowerCurve_point3_activePercentPn(uint8_t unitId, uint16_t value);
bool write_cosPPowerCurve_point3_cosPhi(uint8_t unitId, uint16_t value);
bool write_cosPPowerCurve_point4_activePercentPn(uint8_t unitId, uint16_t value);
bool write_cosPPowerCurve_point4_cosPhi(uint8_t unitId, uint16_t value);
bool write_lockInVoltageForCosPPowerCurve_percentUn(uint8_t unitId, uint16_t value);
bool write_lockOutVoltageForCosPPowerCurve_percentUn(uint8_t unitId, uint16_t value);
bool write_QSetValue_percentSn(uint8_t unitId, uint16_t value);
bool write_QU_curve_point1_U_percentUn(uint8_t unitId, uint16_t value);
bool write_QU_curve_point1_Q_percentSn(uint8_t unitId, uint16_t value);
bool write_QU_curve_point2_U_percentUn(uint8_t unitId, uint16_t value);
bool write_QU_curve_point2_Q_percentSn(uint8_t unitId, uint16_t value);
bool write_QU_curve_point3_U_percentUn(uint8_t unitId, uint16_t value);
bool write_QU_curve_point3_Q_percentSn(uint8_t unitId, uint16_t value);
bool write_QU_curve_point4_U_percentUn(uint8_t unitId, uint16_t value);
bool write_QU_curve_point4_Q_percentSn(uint8_t unitId, uint16_t value);
bool write_lockInPowerForQU_curve_percentPn(uint8_t unitId, uint16_t value);
bool write_lockOutPowerForQU_curve_percentPn(uint8_t unitId, uint16_t value);

bool write_LVRT_TriggerVoltage_percentUn(uint8_t unitId, uint16_t value);
bool write_LVRT_activePowerLimitMode(uint8_t unitId, uint16_t value);

#ifdef __cplusplus
}
#endif