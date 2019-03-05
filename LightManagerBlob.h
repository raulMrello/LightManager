/*
 * LightManagerBlob.h
 *
 *  Created on: Ene 2018
 *      Author: raulMrello
 *
 *	LightManagerBlob es el componente del módulo LightManager en el que se definen los objetos y tipos relativos a
 *	los objetos BLOB de este módulo.
 *	Todos los tipos definidos en este componente están asociados al namespace "Blob", de forma que puedan ser
 *	accesibles mediante el uso de: "Blob::"  e importando este archivo de cabecera.
 */
 
#ifndef LightManagerBlob
#define LightManagerBlob

#include "Blob.h"
#include "mbed.h"

/** Incluye referencia a este módulo para hacer uso de los eventos temporales <Blob::AstCalStatData_t> */
#include "AstCalendarBlob.h"
  


namespace Blob {

/** Valor para identificar acciones vacías, cuyo id es inválido y por lo tanto descartables */
static const int8_t InvalidActionId = -1;

/** Número de puntos de la curva de activación de la carga */
static const uint8_t LightCurveSampleCount = 11;


/** Tamaño máximo de los parámetros textuales */
static const uint16_t LightTextParamLength = 32;


/** Flags para la configuración de notificaciones de un objeto LightManager cuando su configuración se ha
 *  modificado.
 */
 enum LightUpdFlags{
	 EnableLightCfgUpdNotif = (1 << 0),	/// Flag activado para notificar cambios en la configuración en bloque del objeto
 };


 /** Flags de evento que utiliza LightManager para notificar un cambio de estado o alarma
  */
 enum LightEvtFlags{
	 LightNoEvents          = 0,			//!< No hay eventos
 	 LightOutOnEvt			= (1 << 0),		//!< Evento al activar la salida
	 LightOutOffEvt			= (1 << 1),		//!< Evento al desactivar la salida
	 LightOutLevelChangeEvt = (1 << 2),		//!< Evento al cambiar el nivel de activación de la salida
  };


 /** Estructuras de datos para la configuración de rangos de funcionamiento min-max del sensor ALS
  * 	Se forma por diferentes estructuras y tips
  * 	@struct LightMinMax_t Estructura para definir un par min-max
  * 	@struct LightMinMax_t Estructura de configuración del sensor ALS
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



/** Estructuras de datos para la configuración de la salida
 * 	@enum LightOutModeFlags Flags de selección del modo de control de luminaria
 * 	@enum LightActionFlags Flags de configuración de una accción sobre la luminaria
 * 	@struct LigthAction_t Estructura que define una acción a realizar sobre la luminaria
 * 	@struct LightCurve_t Estructura que define la curva de activación (lineal, logarítmica, etc...)
 * 	@struct LightOutData_t Estructura de configuración de la salida de control de la luminaria
 */
enum LightOutModeFlags {
	 LightOutRelayNA = (1 << 0),		//!< Salida de relé normalmente abierto
	 LightOutRelayNC = (1 << 1),		//!< Salida de relé normalmente cerrado
	 LightOutDali	 = (1 << 2),		//!< Salida de control por comandos DALI
	 LightOutPwm	 = (1 << 3),		//!< Salida de control por pwm
	 LightOut010	 = (1 << 4),		//!< Salida de control por voltaje 0-10V
};
enum LightActionFlags {
	LightNoActions 		= 0,			//!< No hay flags de acción
	LightActionSun 		= (1 << 0),		//!< Acción asociada al domingo
	LightActionSat 		= (1 << 1),		//!< Acción asociada al sábado
	LightActionFri 		= (1 << 2),		//!< Acción asociada al viernes
	LightActionThr 		= (1 << 3),		//!< Acción asociada al jueves
	LightActionWed 		= (1 << 4),		//!< Acción asociada al miércoles
	LightActionTue 		= (1 << 5),		//!< Acción asociada al martes
	LightActionMon 		= (1 << 6),		//!< Acción asociada al lunes
	LightActionPeriod0 	= (1 << 7),		//!< Acción asociada al periodo 0
	LightActionPeriod1 	= (1 << 8),		//!< Acción asociada al periodo 1
	LightActionPeriod2 	= (1 << 9),		//!< Acción asociada al periodo 2
	LightActionPeriod3 	= (1 << 10),	//!< Acción asociada al periodo 3
	LightActionPeriod4 	= (1 << 11),	//!< Acción asociada al periodo 4
	LightActionPeriod5 	= (1 << 12),	//!< Acción asociada al periodo 5
	LightActionPeriod6 	= (1 << 13),	//!< Acción asociada al periodo 6
	LightActionPeriod7 	= (1 << 14),	//!< Acción asociada al periodo 7
	LightActionFixTime 	= (1 << 15),	//!< Acción asociada a una hora prefijada
	LightActionFixDate 	= (1 << 16),	//!< Acción asociada a una fecha (ddMM) concreta
	LightActionDawn	 	= (1 << 17),	//!< Acción asociada al orto
	LightActionDusk	 	= (1 << 18),	//!< Acción asociada al ocaso
	LightActionAls	 	= (1 << 19),	//!< Acción asociada al sensor
	LighActionAlsActive = (1 << 20),	//!< Acción asociada al sensor, activa
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
	LightKeyCfgVerbosity= (1 << 6),
	//
	LightKeyCfgAll		= 0x7F,
};


struct __packed LightAction_t {
	int8_t id;							//!< Identificador de la acción
	LightActionFlags flags;				//!< Flags de control de la acción a realizar
	uint16_t date;						//!< Fecha ddMM para activar la acción
	uint16_t time; 						//!< Hora fija de activación (min. día)
	int8_t astCorr;						//!< Corrección sobre el hito astronómico en caso de estar habilitado
	LightMinMax_t luxLevel;				//!< Nivel de luminosidad a partir de la cual se activará
	int8_t outValue;					//!< Valor de salida 0:Off, 1..99:Dimm, 100:On, -1:Acción Desactivada
};
enum LightActionValueRanges{
	LightActionDateMin = 1,
	LightActionDateMax = 31,
	LightActionTimeMax = 1439,
	LightActionAstCorrMax = 1439,
	LightActionOutMax = 100
};
struct __packed LightCurve_t {
	uint16_t samples;					//!< Número de datos de la curva de activación de la luminaria
	int8_t data[LightCurveSampleCount];	//!< datos con la curva de activación (valores +-100) siendo 100 = 1.00f
};

/** Máximo número de acciones en el array de la estructura LightOutData_t. Para aprovechar el máximo número de acciones en la
 *  trama de configuración LightCfgData_t, hay que calcular el espacio disponible, para ello se calcula la variable
 *  MaxAllowedActionDataInArray en función de la trama actual LightOutData_t y la de envío con mayor tamaño, en este caso
 *  es LightCfgData_t
 */
//static const uint32_t MaxAllowedActionDataInArray = (Blob::MaxBlobSize - sizeof(LightAlsData_t) - sizeof(uint8_t) - sizeof(LightCurve_t) - (3 * sizeof(uint32_t)))/sizeof(LightAction_t);
static const uint32_t MaxAllowedActionDataInArray = 20;

struct __packed LightOutData_t{
	LightOutModeFlags mode;				//!< Flags de selección del modo de funcionamiento
	LightCurve_t curve;					//!< Curva de activación
	uint8_t numActions;					//!< Número de acciones en el array
	LightAction_t actions[MaxAllowedActionDataInArray];			//!< Array de acciones
};


/** Estructura de datos para la configuración en bloque del objeto LightManager.
 * 	Se forma por las distintas estructuras de datos de configuración
 * 	@var updFlags Flags de configuración de notificación de cambios de configuración
 * 	@var evtFlags Flags de configuración de notificación de eventos
 * 	@var alsData Datos de ajuste del sensor ALS
 * 	@var outData Datos de ajuste de la salida de control
 * 	@var verbosity Nivel de depuración
 */
struct __packed LightCfgData_t{
	LightUpdFlags updFlagMask;
	LightEvtFlags evtFlagMask;
	LightAlsData_t alsData;
  	LightOutData_t outData;
	esp_log_level_t verbosity;
};


/** Estructura de estado del objeto LightManager notificada tras una medida, una alarma, evento, etc...
 * 	Se forma por las distintas estructuras de datos de configuración
 * 	@var cfg Configuración actualizada
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

/** Estructura de eventos temporales. Clona el objeto proporcionado por AstCalendar */
typedef Blob::AstCalStatData_t LightTimeData_t;

}	// end namespace Blob


namespace JSON {

/**
 * Codifica la configuración actual en un objeto JSON
 * @param cfg Configuración
 * @return Objeto JSON o NULL en caso de error
 */
cJSON* getJsonFromLightCfg(const Blob::LightCfgData_t& cfg);

/**
 * Codifica el estado actual en un objeto JSON
 * @param stat Estado
 * @return Objeto JSON o NULL en caso de error
 */
cJSON* getJsonFromLightStat(const Blob::LightStatData_t& stat);

/**
 * Codifica el estado de arranque en un objeto JSON
 * @param boot Estado de arranque
 * @return Objeto JSON o NULL en caso de error
 */
cJSON* getJsonFromLightBoot(const Blob::LightBootData_t& boot);

/**
 * Codifica la configuración de la célula ALS en un objeto JSON
 * @param lux Configuración ALS
 * @return Objeto JSON o NULL en caso de error
 */
cJSON* getJsonFromLightLux(const Blob::LightLuxLevel& lux);

/**
 * Codifica un evento temporal en un objeto JSON
 * @param t Evento temporal
 * @return Objeto JSON o NULL en caso de error
 */
cJSON* getJsonFromLightTime(const Blob::LightTimeData_t& t);

/**
 * Decodifica el mensaje JSON en un objeto de configuración
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Parámetros decodificados o 0 en caso de error
 */
uint32_t getLightCfgFromJson(Blob::LightCfgData_t &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto de estado
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Parámetros decodificados o 0 en caso de error
 */
uint32_t getLightStatFromJson(Blob::LightStatData_t &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto de arranque
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Parámetros decodificados o 0 en caso de error
 */
uint32_t getLightBootFromJson(Blob::LightBootData_t &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto de configuración ALS
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Parámetros decodificados o 0 en caso de error
 */
uint32_t getLightLuxFromJson(Blob::LightLuxLevel &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un evento temporal
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Parámetros decodificados o 0 en caso de error
 */
uint32_t getLightTimeFromJson(Blob::LightTimeData_t &obj, cJSON* json);


}	// end namespace JSON



#endif
