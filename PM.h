#ifndef PM_H
#define PM_H

#include "FITboardsCommon.h"

struct TypePM {
    struct ActualValues{
        static const quint8// size = end_address + 1 - start_address
            block0addr = 0x00, block0size = 0x7D + 1 - block0addr, //126
            block1addr = 0x7F, block1size = 0xBD + 1 - block1addr, // 63
			block2addr = 0xFC, block2size = 0xFE + 1 - block2addr; //  3

        union { //block0
            quint32 registers0[block0size] = {0};
            char pointer0[block0size * sizeof(quint32)];
            struct {
                quint32 OR_GATE               , //]00
                        TIME_ALIGN  [12]      , //]01-0C
                        ADC_BASELINE[12][2]   , //]0D-24 //[Ch][0] for ADC0, [Ch][1] for ADC1
                        ADC_RANGE   [12][2]   , //]25-3C
                        CFD_SATR              ; //]3D
                qint32  TDC1tuning         : 8, //┐
                        TDC2tuning         : 8, //│3E
                                           :16, //┘
                        TDC3tuning         : 8, //┐
                                           :24; //┘3F
                quint8  RAW_TDC_DATA[12][4]   ; //]40-4B //[Ch][0] for val1, [Ch][1] for val2
                quint32 DISPERSION  [12][2]   ; //]4C-63
                qint16  MEANAMPL    [12][2][2]; //]64-7B //[Ch][0][0] for ADC0, [Ch][1][0] for ADC1
                quint32 CH_MASK               , //]7C
                        CH_DISPLACED          ; //]7D
            };
        };
        union { //block1
            quint32 registers1[block1size] = {0};
            char pointer1[block1size * sizeof(quint32)];
            struct {
                quint32 mainPLLlocked      : 1, //┐
                        TDC1PLLlocked      : 1, //│
                        TDC2PLLlocked      : 1, //│
                        TDC3PLLlocked      : 1, //│
                        GBTlinkPresent     : 1, //│
                        GBTreceiverError   : 1, //│
                        TDC1syncError      : 1, //│
                        TDC2syncError      : 1, //│
                        TDC3syncError      : 1, //│7F
                        RESET_COUNTERS     : 1, //│
                                           : 2, //│
                        GBTRxPhaseError    : 1, //│
                        BCIDsyncLost       : 1, //│
                        droppingHits       : 1, //│
                                           :17; //┘
                struct ChannelSettings {        //┐
                    quint32 CFD_THRESHOLD  :16, //│
                                           :16; //│
                    qint32  CFD_ZERO       :16, //│
                                           :16, //│80-AF
                            ADC_ZERO       :16, //│
                                           :16; //│
                    quint32 ADC_DELAY      :16, //│
                                           :16; //│
                } Ch[12];                       //┘
                qint32  THRESHOLD_CALIBR[12]  , //]B0-BB
						TEMPERATURE		   :16, //┐
										   :16; //┘BC
				quint32 MCODE_VERSION      : 8, //┐
                        SERIAL_NUM         : 8, //│BD
                                           :16; //┘
            };
        };
        GBTunit GBT;                            //]D8-EF
		Timestamp MCODE_TIME;					//]F7
        union { //block2
            quint32 registers2[block2size] = {0};
            char pointer2[block2size * sizeof(quint32)];
            struct {
                quint32 FPGA_TEMP,              //]FC
                        POWER_1V,               //]FD
                        POWER_1_8V;             //]FE
            };
        };
		Timestamp FW_TIME;						//]FF
        double //calculable parameters
            temp_board,
            temp_FPGA,
            voltage_1V,
            voltage_1_8V;
        void calculateDoubleValues() {
            temp_board   = TEMPERATURE / 10.;
            temp_FPGA    = FPGA_TEMP  * 503.975 / 65536 - 273.15;
            voltage_1V   = POWER_1V   * 3. / 65536;
            voltage_1_8V = POWER_1_8V * 3. / 65536;
        }
    } act;

    struct Settings {
        static const quint8// size = end_address + 1 - start_address
            block0addr = 0x00, block0size = 0x0C + 1 - block0addr, //13
            block1addr = 0x25, block1size = 0x3D + 1 - block1addr, //25
            block2addr = 0x80, block2size = 0xBB + 1 - block2addr; //60

        union { //block0
            quint32 registers0[block0size] = {0};
            char pointer0[block0size * sizeof(quint32)];
            struct {
                quint32 OR_GATE                , //]00
                        TIME_ALIGN  [12]       ; //]01-0C
            };
        };
        union { //block1
            quint32 registers1[block1size] = {0};
            char pointer1[block1size * sizeof(quint32)];
            struct {
                quint32 ADC_RANGE   [12][2]    , //]25-3C
                        CFD_SATR               ; //]3D
            };
        };
        quint32 CH_MASK;                         //]7C
        union { //block2
            quint32 registers2[block2size] = {0};
            char pointer2[block2size * sizeof(quint32)];
            struct {
                struct ChannelSettings {        //┐
                    quint32 CFD_THRESHOLD  :16, //│
                                           :16; //│
                    qint32  CFD_ZERO       :16, //│
                                           :16, //│80-AF
                            ADC_ZERO       :16, //│
                                           :16; //│
                    quint32 ADC_DELAY      :16, //│
                                           :16; //│
                } Ch[12];                       //┘
				quint32  THRESHOLD_CALIBR[12];  //]B0-BB
            };
        };
        GBTunit::ControlData GBT;               //]D8-E7
    } set;

    struct Counters {
        static const quint16
            addressFIFO     = 0x100,
            addressFIFOload = 0x101;
        static const quint8
            number = 24,
            addressDirect   =  0xC0;
        quint16 FIFOload;
        QDateTime newTime, oldTime;
        union {
            quint32 New[number] = {0};
            struct {
                quint32 CFD,
                        TRG;
            } Ch[12];
        };
        quint32 Old[number] = {0};
        union {
			double rate[number] = {0.};
            struct {
				double CFD,
					   TRG;
            } rateCh[12];
        };
    } counters;

	//quint16 &FEEid = *((quint16 *)(set.GBT.registers+9) + 1);
	const quint16 baseAddress;
    const QString name;
    TypePM(quint16 addr, QString PMname) : baseAddress(addr), name(PMname) {}
};

const QHash<QString, Parameter> PMparameters = {
    //name                  address width shift interval
    {"OR_GATE"              ,  0x00               },
    {"TIME_ALIGN"           , {0x01, 32,  0,    1}},
    {"ADC0_OFFSET"          , {0x0D, 32,  0,    2}},
    {"ADC1_OFFSET"          , {0x0E, 32,  0,    2}},
    {"ADC0_RANGE"           , {0x25, 32,  0,    2}},
    {"ADC1_RANGE"           , {0x26, 32,  0,    2}},
    {"CFD_SATR"             ,  0x3D               },
    {"CH_MASK"              ,  0x7C               },
    {"CFD_THRESHOLD"        , {0x80, 32,  0,    4}},
    {"CFD_ZERO"             , {0x81, 32,  0,    4}},
    {"ADC_ZERO"             , {0x82, 32,  0,    4}},
    {"ADC_DELAY"            , {0x83, 32,  0,    4}},
    {"THRESHOLD_CALIBR"     , {0xB0, 32,  0,    1}},
    {"DG_MODE"              , {0xD8,  4,  0}      },
    {"TG_MODE"              , {0xD8,  4,  4}      },
    {"HB_RESPONSE"          , {0xD8,  1, 20}      },
    {"BYPASS_MODE"          , {0xD8,  1, 21}      },
    {"READOUT_LOCK"         , {0xD8,  1, 22}      },
    {"DG_TRG_RESPOND_MASK"  ,  0xD9               },
    {"DG_BUNCH_PATTERN"     ,  0xDA               },
    {"TG_PATTERN_1"         ,  0xDC               },
    {"TG_PATTERN_0"         ,  0xDD               },
    {"TG_CONT_VALUE"        ,  0xDE               },
    {"DG_BUNCH_FREQ"        ,  0xDF               },
    {"TG_BUNCH_FREQ"        , {0xDF, 16, 16}      },
    {"DG_FREQ_OFFSET"       , {0xE0, 12,  0}      },
    {"TG_FREQ_OFFSET"       , {0xE0, 12, 16}      },
    {"RDH_PAR"              , {0xE1, 16,  0}      },
    {"RDH_FEE_ID"           , {0xE1, 16, 16}      },
    {"RDH_DET_FIELD"        , {0xE2, 16,  0}      },
    {"RDH_MAX_PAYLOAD"      , {0xE2, 16, 16}      },
    {"BCID_DELAY"           , {0xE3, 12,  0}      },
    {"CRU_TRG_COMPARE_DELAY", {0xE3, 12, 16}      },
    {"DATA_SEL_TRG_MASK"    ,  0xE4               }
};

#endif // PM_H
