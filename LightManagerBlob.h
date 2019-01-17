/*
 * LightManagerBlob.h
 *
 *  Created on: Ene 2018
 *      Author: raulMrello
 *
 *	LightManagerBlob es el componente del m�dulo LightManager en el que se definen los objetos y tipos relativos a
 *	los objetos BLOB de este m�dulo.
 *	Todos los tipos definidos en este componente est�n asociados al namespace "Blob", de forma que puedan ser
 *	accesibles mediante el uso de: "Blob::"  e importando este archivo de cabecera.
 */
 
#ifndef LightManagerBlob
#define LightManagerBlob

#include "Blob.h"
#include "mbed.h"
  


namespace Blob {

/** Valor para identificar acciones vac�as, cuyo id es inv�lido y por lo tanto descartables */
static const int8_t InvalidActionId = -1;

/** N�mero de puntos de la curva de activaci�n de la carga */
static const uint8_t LightCurveSampleCount = 11;


/** Tama�o m�ximo de los par�metros textuales */
static const uint16_t LightTextParamLength = 32;


/** Flags para la configuraci�n de notificaciones de un objeto LightManager cuando su configuraci�n se ha
 *  modificado.
 */
 enum LightUpdFlags{
	 EnableLightCfgUpdNotif = (1 << 0),	/// Flag activado para notificar cambios en la configuraci�n en bloque del objeto
 };


 /** Flags de evento que utiliza LightManager para notificar un cambio de estado o alarma
  */
 enum LightEvtFlags{
	 LightNoEvents          = 0,			//!< No hay eventos
 	 LightOutOnEvt			= (1 << 0),		//!< Evento al activar la salida
	 LightOutOffEvt			= (1 << 1),		//!< Evento al desactivar la salida
	 LightOutLevelChangeEvt = (1 << 2),		//!< Evento al cambiar el nivel de activaci�n de la salida
  };


 /** Estructuras de datos para la configuraci�n de rangos de funcionamiento min-max del sensor ALS
  * 	Se forma por diferentes estructuras y tips
  * 	@struct LightMinMax_t Estructura para definir un par min-max
  * 	@struct LightMinMax_t Estructura de configuraci�n del sensor ALS
  */
typedef uint32_t LightLuxLevel;
struct __packed LightMinMax_t{
	LightLuxLevel min;
	LightLuxLevel max;
	LightLuxLevel thres;
 };
struct __packed LightAlsData_t{
	LightMinMax_t lux;				//!< Rangos de luminosidad en lux
};



/** Estructuras de datos para la configuraci�n de la salida
 * 	@enum LightOutModeFlags Flags de selecci�n del modo de control de luminaria
 * 	@enum LightActionFlags Flags de configuraci�n de una accci�n sobre la luminaria
 * 	@struct LigthAction_t Estructura que define una acci�n a realizar sobre la luminaria
 * 	@struct LightCurve_t Estructura que define la curva de activaci�n (lineal, logar�tmica, etc...)
 * 	@struct LightOutData_t Estructura de configuraci�n de la salida de control de la luminaria
 */
enum LightOutModeFlags {
	 LightOutRelayNA = (1 << 0),		//!< Salida de rel� normalmente abierto
	 LightOutRelayNC = (1 << 1),		//!< Salida de rel� normalmente cerrado
	 LightOutDali	 = (1 << 2),		//!< Salida de control por comandos DALI
	 LightOutPwm	 = (1 << 3),		//!< Salida de control por pwm
	 LightOut010	 = (1 << 4),		//!< Salida de control por voltaje 0-10V
};
enum LightActionFlags {
	LightNoActions 		= 0,			//!< No hay flags de acci�n
	LightActionSun 		= (1 << 0),		//!< Acci�n asociada al domingo
	LightActionSat 		= (1 << 1),		//!< Acci�n asociada al s�bado
	LightActionFri 		= (1 << 2),		//!< Acci�n asociada al viernes
	LightActionThr 		= (1 << 3),		//!< Acci�n asociada al jueves
	LightActionWed 		= (1 << 4),		//!< Acci�n asociada al mi�rcoles
	LightActionTue 		= (1 << 5),		//!< Acci�n asociada al martes
	LightActionMon 		= (1 << 6),		//!< Acci�n asociada al lunes
	LightActionPeriod0 	= (1 << 7),		//!< Acci�n asociada al periodo 0
	LightActionPeriod1 	= (1 << 8),		//!< Acci�n asociada al periodo 1
	LightActionPeriod2 	= (1 << 9),		//!< Acci�n asociada al periodo 2
	LightActionPeriod3 	= (1 << 10),	//!< Acci�n asociada al periodo 3
	LightActionPeriod4 	= (1 << 11),	//!< Acci�n asociada al periodo 4
	LightActionPeriod5 	= (1 << 12),	//!< Acci�n asociada al periodo 5
	LightActionPeriod6 	= (1 << 13),	//!< Acci�n asociada al periodo 6
	LightActionPeriod7 	= (1 << 14),	//!< Acci�n asociada al periodo 7
	LightActionFixTime 	= (1 << 15),	//!< Acci�n asociada a una hora prefijada
	LightActionFixDate 	= (1 << 16),	//!< Acci�n asociada a una fecha (ddMM) concreta
	LightActionDawn	 	= (1 << 17),	//!< Acci�n asociada al orto
	LightActionDusk	 	= (1 << 18),	//!< Acci�n asociada al ocaso
	LightActionAls	 	= (1 << 19),	//!< Acci�n asociada al sensor
	LighActionAlsActive = (1 << 20),	//!< Acci�n asociada al sensor, activa
};


/** Flags para identificar cada key-value de los objetos JSON que se han modificado en un SET remoto
 */
enum LightKeyNames{
	LightKeyNone 		= 0,
	LightKeyCfgUpd		= (1 << 0),
	LightKeyCfgEvt		= (1 << 1),
	LightKeyCfgAls		= (1 << 2),
	LightKeyCfgOutm		= (1 << 3),
	LightKeyCfgCurve	= (1 << 4),
	LightKeyCfgActs		= (1 << 5),
};


struct __packed LightAction_t {
	int8_t id;							//!< Identificador de la acci�n
	LightActionFlags flags;				//!< Flags de control de la acci�n a realizar
	uint16_t date;						//!< Fecha ddMM para activar la acci�n
	uint16_t time; 						//!< Hora fija de activaci�n (min. d�a)
	int8_t astCorr;						//!< Correcci�n sobre el hito astron�mico en caso de estar habilitado
	LightMinMax_t luxLevel;				//!< Nivel de luminosidad a partir de la cual se activar�
	int8_t outValue;					//!< Valor de salida 0:Off, 1..99:Dimm, 100:On, -1:Acci�n Desactivada
};
enum LightActionValueRanges{
	LightActionDateMin = 1,
	LightActionDateMax = 31,
	LightActionTimeMax = 1439,
	LightActionAstCorrMax = 1439,
	LightActionOutMax = 100
};
struct __packed LightCurve_t {
	uint16_t samples;					//!< N�mero de datos de la curva de activaci�n de la luminaria
	int8_t data[LightCurveSampleCount];	//!< datos con la curva de activaci�n (valores +-100) siendo 100 = 1.00f
};

/** M�ximo n�mero de acciones en el array de la estructura LightOutData_t. Para aprovechar el m�ximo n�mero de acciones en la
 *  trama de configuraci�n LightCfgData_t, hay que calcular el espacio disponible, para ello se calcula la variable
 *  MaxAllowedActionDataInArray en funci�n de la trama actual LightOutData_t y la de env�o con mayor tama�o, en este caso
 *  es LightCfgData_t
 */
//static const uint32_t MaxAllowedActionDataInArray = (Blob::MaxBlobSize - sizeof(LightAlsData_t) - sizeof(uint8_t) - sizeof(LightCurve_t) - (3 * sizeof(uint32_t)))/sizeof(LightAction_t);
static const uint32_t MaxAllowedActionDataInArray = 20;

struct __packed LightOutData_t{
	LightOutModeFlags mode;				//!< Flags de selecci�n del modo de funcionamiento
	LightCurve_t curve;					//!< Curva de activaci�n
	uint8_t numActions;					//!< N�mero de acciones en el array
	LightAction_t actions[MaxAllowedActionDataInArray];			//!< Array de acciones
};


  /** Estructura de datos para la configuraci�n en bloque del objeto LightManager.
   * 	Se forma por las distintas estructuras de datos de configuraci�n
   * 	@var updFlags Flags de configuraci�n de notificaci�n de cambios de configuraci�n
   * 	@var evtFlags Flags de configuraci�n de notificaci�n de eventos
   * 	@var alsData Datos de ajuste del sensor ALS
   * 	@var outData Datos de ajuste de la salida de control
   */
struct __packed LightCfgData_t{
	LightUpdFlags updFlagMask;
	LightEvtFlags evtFlagMask;
	LightAlsData_t alsData;
  	LightOutData_t outData;
  	uint32_t keys;
};


/** Estructura de estado del objeto LightManager notificada tras una medida, una alarma, evento, etc...
 * 	Se forma por las distintas estructuras de datos de configuraci�n
 * 	@var cfg Configuraci�n actualizada
 * 	@var flags Flags de estado
 * 	@var outValue Valor de la salida de control
 */
struct __packed LightStatData_t{
	uint32_t flags;
	uint8_t outValue;
};


/** Estructura de datos asociado al boot
 */
struct __packed LightBootData_t{
	LightCfgData_t cfg;
	LightStatData_t stat;
};



}



#endif
