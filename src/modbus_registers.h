#pragma once
#include <Arduino.h>
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

/* Read wrappers (input & holding read requests) */
/* Input registers (chapter 3.3) - prototypes */
void deviceType(uint8_t slave);
void modbusAddress(uint8_t slave);
void serialNumber(uint8_t slave);
void machineType(uint8_t slave);
void currentGridCode(uint8_t slave);
void ratedPower_W(uint8_t slave);
void softwareVersion(uint8_t slave);
void safetyVersion(uint8_t slave);
void manufacturerName(uint8_t slave);
void brandName(uint8_t slave);

void gridRatedVoltage_V(uint8_t slave);
void gridRatedFrequency_Hz(uint8_t slave);
void eToday_kWh(uint8_t slave);
void eTotal_kWh(uint8_t slave);
void hTotal_H(uint8_t slave);
void deviceState(uint8_t slave);
void connectTime_s(uint8_t slave);

void airTemperature_C(uint8_t slave);
void inverterUphaseTemperature_C(uint8_t slave);
void inverterVphaseTemperature_C(uint8_t slave);
void inverterWphaseTemperature_C(uint8_t slave);
void boostTemperature_C(uint8_t slave);
void bidirectionalDCDCtemperature_C(uint8_t slave);
void busVoltage_V(uint8_t slave);

void pv1Voltage_V(uint8_t slave);
void pv1Current_A(uint8_t slave);
void pv2Voltage_V(uint8_t slave);
void pv2Current_A(uint8_t slave);
void pv3Voltage_V(uint8_t slave);
void pv3Current_A(uint8_t slave);
void pv4Voltage_V(uint8_t slave);
void pv4Current_A(uint8_t slave);
void pv5Voltage_V(uint8_t slave);
void pv5Current_A(uint8_t slave);

void string1Current_A(uint8_t slave);
void string2Current_A(uint8_t slave);
void string3Current_A(uint8_t slave);
void string4Current_A(uint8_t slave);
void string5Current_A(uint8_t slave);
void string6Current_A(uint8_t slave);
void string7Current_A(uint8_t slave);
void string8Current_A(uint8_t slave);
void string9Current_A(uint8_t slave);
void string10Current_A(uint8_t slave);

void L1PhaseVoltage_V(uint8_t slave);
void L1PhaseCurrent_A(uint8_t slave);
void L2PhaseVoltage_V(uint8_t slave);
void L2PhaseCurrent_A(uint8_t slave);
void L3PhaseVoltage_V(uint8_t slave);
void L3PhaseCurrent_A(uint8_t slave);
void RSLineVoltage_V(uint8_t slave);
void RTLineVoltage_V(uint8_t slave);
void STLineVoltage_V(uint8_t slave);

void gridFrequency_Hz(uint8_t slave);
void apparentPower_VA(uint8_t slave);
void activePower_W(uint8_t slave);
void reactivePower_Var(uint8_t slave);
void powerFactor(uint8_t slave);

void errorCode(uint8_t slave);
void warningCode(uint8_t slave);

// PV / battery related (selected)
void pvTotalPower_W(uint8_t slave);
void pvEToday_kWh(uint8_t slave);
void pvETotal_kWh(uint8_t slave);
void batteryCommunicationStatus(uint8_t slave);
void batteryStatus(uint8_t slave);
void batteryErrorStatus(uint8_t slave);
void batteryWarningStatus(uint8_t slave);
void batteryVoltage_V(uint8_t slave);
void batteryCurrent_A(uint8_t slave);
void batteryPower_W(uint8_t slave);
void batteryTemperature_C(uint8_t slave);
void batterySOC(uint8_t slave);
void batterySOH(uint8_t slave);
void batteryChargingCurrentLimit_A(uint8_t slave);
void batteryDischargeCurrentLimit_A(uint8_t slave);
void batteryEChargeToday_kWh(uint8_t slave);
void batteryEDischargeToday_kWh(uint8_t slave);
void eConsumptionToday_AC_kWh(uint8_t slave);
void eGenerationToday_AC_kWh(uint8_t slave);
void EPSLoadVoltage_V(uint8_t slave);
void EPSLoadCurrent_A(uint8_t slave);
void EPSLoadFrequency_Hz(uint8_t slave);
void EPSLoadActivePower_W(uint8_t slave);
void EPSLoadReactivePower_Var(uint8_t slave);
void eConsumptionToday_EPS_kWh(uint8_t slave);
void eConsumptionTotal_EPS_kWh(uint8_t slave);

/* Holding registers (read wrappers) */
void remoteSwitchCommand(uint8_t slave);
void rtc_Year(uint8_t slave);
void rtc_Month(uint8_t slave);
void rtc_Day(uint8_t slave);
void rtc_Hour(uint8_t slave);
void rtc_Minute(uint8_t slave);
void rtc_Seconds(uint8_t slave);

void storageInverterSwitch(uint8_t slave);
void typeSelectionOfEnergyStorageMachine(uint8_t slave);
void runMode(uint8_t slave);
void batteryManufacturer(uint8_t slave);
void smartMeterStatus(uint8_t slave);
void smartMeterAdjustmentFlag(uint8_t slave);
void setTargetPowerValue_W(uint8_t slave);
void currentPowerValueOfSmartMeter_W(uint8_t slave);
void antiReverseCurrentFlag(uint8_t slave);
void batteryWakeUp(uint8_t slave);
void commboxAndCloudCommunicationStatus(uint8_t slave);
void chargeDischargeFlagBit(uint8_t slave);
void chargeAndDischargePowerCommand_W(uint8_t slave);

void activePowerControlFunction(uint8_t slave);
void eegControlFunction(uint8_t slave);
void slopeLoadFunction(uint8_t slave);
void overvoltageReducePowerFunction(uint8_t slave);
void overfrequencyReducePowerFunction(uint8_t slave);
void reactivePowerControlFunction(uint8_t slave);
void LVRTFunction(uint8_t slave);
void tenMinutesAverageOvervoltageProtectFunction(uint8_t slave);
void islandingProtectFunction(uint8_t slave);
void peConnectionCheckFunction(uint8_t slave);
void overloadFunction(uint8_t slave);
void shadowMPPTFunction(uint8_t slave);

/* 45xxx & 454xx & 455xx groups (read) */
void gridCode(uint8_t slave);
void overvoltageProtectionValue3_V(uint8_t slave);
void overvoltageProtectionValue2_V(uint8_t slave);
void overvoltageProtectionValueFreq_Hz(uint8_t slave);
void underfrequencyProtectionValue_Hz(uint8_t slave);
void gridVoltageHighLimit3_V(uint8_t slave);
void gridVoltageHighLimitTime3_ms(uint8_t slave);
void gridVoltageHighLimit2_V(uint8_t slave);
void gridVoltageHighLimitTime2_ms(uint8_t slave);
void gridVoltageHighLimit1_V(uint8_t slave);
void gridVoltageHighLimitTime1_ms(uint8_t slave);
void gridVoltageLowLimit3_V(uint8_t slave);
void gridVoltageLowLimitTime3_ms(uint8_t slave);
void gridVoltageLowLimit2_V(uint8_t slave);
void gridVoltageLowLimitTime2_ms(uint8_t slave);
void gridVoltageLowLimit1_V(uint8_t slave);
void gridVoltageLowLimitTime1_ms(uint8_t slave);
void tenMinutesAverageOvervoltageThreshold_V(uint8_t slave);
void tenMinutesAverageOvervoltageProtectTime_ms(uint8_t slave);
void overvoltageRecoverValue_V(uint8_t slave);
void undervoltageRecoverValue_V(uint8_t slave);
void gridFrequencyHighLimit3_Hz(uint8_t slave);
void gridFrequencyHighLimitTime3_ms(uint8_t slave);
void gridFrequencyHighLimit2_Hz(uint8_t slave);
void gridFrequencyHighLimitTime2_ms(uint8_t slave);
void gridFrequencyHighLimit1_Hz(uint8_t slave);
void gridFrequencyHighLimitTime1_ms(uint8_t slave);
void gridFrequencyLowLimit3_Hz(uint8_t slave);
void gridFrequencyLowLimitTime3_ms(uint8_t slave);
void gridFrequencyLowLimit2_Hz(uint8_t slave);
void gridFrequencyLowLimitTime2_ms(uint8_t slave);
void gridFrequencyLowLimit1_Hz(uint8_t slave);
void gridFrequencyLowLimitTime1_ms(uint8_t slave);
void varyRateOfFrequencyProtectValue_HzPerS(uint8_t slave);
void varyRateOfFrequencyProtectTime_ms(uint8_t slave);
void overfrequencyRecoverValue_Hz(uint8_t slave);
void underfrequencyRecoverValue_Hz(uint8_t slave);
void timeOfFirstConnectionToGrid_s(uint8_t slave);
void timeOfReconnectionToGrid_s(uint8_t slave);
void isoProtectThreshold_kOhm(uint8_t slave);
void dciProtectThreshold_mA(uint8_t slave);
void dciProtectTime_ms(uint8_t slave);

void loadRateOfFirstConnectionToGrid(uint8_t slave);
void loadRateOfReconnectionToGrid(uint8_t slave);
void activePowerSet_percentPn(uint8_t slave);
void increaseRateOfActivePower_percentPnPerMin(uint8_t slave);
void decreaseRateOfActivePower_percentPnPerMin(uint8_t slave);
void overFrequencyReducePowerMode(uint8_t slave);
void overFrequencyReducePowerStartFrequency_Hz(uint8_t slave);
void overFrequencyReducePowerStopFrequency_Hz(uint8_t slave);
void overFrequencyReducePowerBackFrequency_Hz(uint8_t slave);
void reduceRatioOfOverFrequencyReducePower(uint8_t slave);
void overFrequencyReducePowerReduceDelayTime_s(uint8_t slave);
void overFrequencyReducePowerRecoverDelayTime_s(uint8_t slave);
void speedOfOverFrequencyRecoverToPn(uint8_t slave);
void overFrequencyZeroPowerFrequencyPoint_Hz(uint8_t slave);

void overVoltageReducePowerMode(uint8_t slave);
void overVoltageReducePowerStartVoltage_percentUn(uint8_t slave);
void overVoltageReducePowerStopVoltage_percentUn(uint8_t slave);
void overVoltageReducePowerBackVoltage_percentUn(uint8_t slave);
void reduceRatioOfOverVoltageReducePower(uint8_t slave);
void overVoltageReducePowerDelayTime_s(uint8_t slave);
void overVoltageRecoverPowerDelayTime_s(uint8_t slave);
void speedOfOverVoltageRecoverToPn(uint8_t slave);

void underFrequencyIncreasePowerMode(uint8_t slave);
void underFrequencyIncreasePowerStartFrequency_Hz(uint8_t slave);
void underFrequencyIncreasePowerStopFrequency_Hz(uint8_t slave);
void underFrequencyIncreasePowerBackFrequency_Hz(uint8_t slave);
void increaseRatioOfUnderFrequencyIncreasePower(uint8_t slave);
void underFrequencyIncreasePowerDelayTime_s(uint8_t slave);
void underFrequencyRecoverPowerDelayTime_s(uint8_t slave);
void speedOfUnderFrequencyRecoverToPn(uint8_t slave);
void underFrequencyZeroPowerFrequencyPoint_Hz(uint8_t slave);

void underVoltageIncreasePowerMode(uint8_t slave);
void underVoltageIncreasePowerStartVoltage_percentUn(uint8_t slave);
void underVoltageIncreasePowerStopVoltage_percentUn(uint8_t slave);
void underVoltageIncreasePowerBackVoltage_percentUn(uint8_t slave);
void increaseRatioOfUnderVoltageIncreasePower(uint8_t slave);
void underVoltageIncreasePowerDelayTime_s(uint8_t slave);
void underVoltageIncreasePowerDelayTime2_s(uint8_t slave);
void speedOfUnderVoltageRecoverToPn(uint8_t slave);

void pav_percentPn(uint8_t slave);
void drmsPval_percentPn(uint8_t slave);

void reactivePowerControlMode(uint8_t slave);
void timeConstantOfReactivePowerCurve_s(uint8_t slave);
void pfSetValue(uint8_t slave); // 45503
void cosPPowerCurve_point1_activePercentPn(uint8_t slave);
void cosPPowerCurve_point1_cosPhi(uint8_t slave);
void cosPPowerCurve_point2_activePercentPn(uint8_t slave);
void cosPPowerCurve_point2_cosPhi(uint8_t slave);
void cosPPowerCurve_point3_activePercentPn(uint8_t slave);
void cosPPowerCurve_point3_cosPhi(uint8_t slave);
void cosPPowerCurve_point4_activePercentPn(uint8_t slave);
void cosPPowerCurve_point4_cosPhi(uint8_t slave);
void lockInVoltageForCosPPowerCurve_percentUn(uint8_t slave);
void lockOutVoltageForCosPPowerCurve_percentUn(uint8_t slave);
void QSetValue_percentSn(uint8_t slave);
void QU_curve_point1_U_percentUn(uint8_t slave);
void QU_curve_point1_Q_percentSn(uint8_t slave);
void QU_curve_point2_U_percentUn(uint8_t slave);
void QU_curve_point2_Q_percentSn(uint8_t slave);
void QU_curve_point3_U_percentUn(uint8_t slave);
void QU_curve_point3_Q_percentSn(uint8_t slave);
void QU_curve_point4_U_percentUn(uint8_t slave);
void QU_curve_point4_Q_percentSn(uint8_t slave);
void lockInPowerForQU_curve_percentPn(uint8_t slave);
void lockOutPowerForQU_curve_percentPn(uint8_t slave);

void LVRT_TriggerVoltage_percentUn(uint8_t slave);
void LVRT_activePowerLimitMode(uint8_t slave);

/* --- write helpers --- */
/* Generic single and double register writers */
bool writeRegisterU16(uint8_t slave, uint32_t addr_dec, uint16_t value);
bool writeRegisterU32(uint8_t slave, uint32_t addr_dec, uint32_t value);

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