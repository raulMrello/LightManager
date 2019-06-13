/*
 * LightManager.h
 *
 *  Created on: Ene 2018
 *      Author: raulMrello
 *
 *	LightManager es el módulo encargado de gestionar la salida de control de la luminaria
 */
 
#ifndef __LightManager__H
#define __LightManager__H

#include "mbed.h"
#include "ActiveModule.h"
#include "LightManagerBlob.h"
#include "Scheduler.h"
#include "Driver_Pwm010.h"
#include "JsonParserBlob.h"

/** Flag para habilitar el soporte de objetos JSON en las suscripciones a MQLib
 *  Por defecto DESACTIVADO
 */
#define LIGHTMANAGER_ENABLE_JSON_SUPPORT		0


   
class LightManager : public ActiveModule {
  public:

    static const uint32_t MaxNumMessages = 16;		//!< Máximo número de mensajes procesables en el Mailbox del componente
              
    /** Constructor por defecto
     *  @param pin010 Pin del Driver de control 0-10
     * 	@param fs Objeto FSManager para operaciones de backup
     * 	@param defdbg Flag para habilitar depuración por defecto
     */
    LightManager(PinName pin010, FSManager* fs, bool defdbg = false);


    /** Destructor
     */
    virtual ~LightManager(){}


    /** Arranca el simulador de eventos basándose en un RtosTimer
     *
     */
    void startSimulator();


    /** Detiene el simulador de eventos basándose en un RtosTimer
     *
     */
    void stopSimulator();


    /** Interfaz para postear un mensaje de la máquina de estados en el Mailbox de la clase heredera
     *  @param msg Mensaje a postear
     *  @return Resultado
     */
    virtual osStatus putMessage(State::Msg *msg);


    /**
     * Activa y/o desactiva el soporte JSON
     * @param flag
     */
    void setJSONSupport(bool flag){
    	_json_supported = flag;
    }


    /**
     * Obtiene el estado del soporte JSON
     * @return
     */
    bool isJSONSupported(){
    	return _json_supported;
    }

  private:

    /** Máximo número de mensajes alojables en la cola asociada a la máquina de estados */
    static const uint32_t MaxQueueMessages = 16;

    /** Flags de operaciones a realizar por la tarea */
    enum MsgEventFlags{
    	RecvCfgSet 	  = (State::EV_RESERVED_USER << 0),  /// Flag activado al recibir mensaje en "cmd/$BASE/cfg/set"
    	RecvCfgGet	  = (State::EV_RESERVED_USER << 1),  /// Flag activado al recibir mensaje en "cmd/$BASE/cfg/get"
    	RecvStatSet	  = (State::EV_RESERVED_USER << 2),  /// Flag activado al recibir mensaje en "cmd/$BASE/value/set"
    	RecvStatGet	  = (State::EV_RESERVED_USER << 3),  /// Flag activado al recibir mensaje en "cmd/$BASE/value/get"
    	RecvBootGet	  = (State::EV_RESERVED_USER << 4),  /// Flag activado al recibir mensaje en "get/boot"
    	RecvTimeSet	  = (State::EV_RESERVED_USER << 5),  /// Flag activado al recibir mensaje en "set/time"
    	RecvLuxSet	  = (State::EV_RESERVED_USER << 6),  /// Flag activado al recibir mensaje en "set/lux"
    };


    /** Cola de mensajes de la máquina de estados */
    Queue<State::Msg, MaxQueueMessages> _queue;

    /** Datos de configuración y estado */
    Blob::LightBootData_t _lightdata;

    /** Scheduler de programas */
    Scheduler* _sched;

    /** Timer de simulación de eventos */
    RtosTimer* _sim_tmr;

    /** Contador de segundos del simulador de eventos */
    uint32_t _sim_counter;

    /** Driver de control 0-10 */
    Driver_Pwm010* _driver010;

    /** Flag de control para el soporte de objetos json */
    bool _json_supported;

    /** Interfaz para obtener un evento osEvent de la clase heredera
     *  @param msg Mensaje a postear
     */
    virtual osEvent getOsEvent();


 	/** Interfaz para manejar los eventos en la máquina de estados por defecto
      *  @param se Evento a manejar
      *  @return State::StateResult Resultado del manejo del evento
      */
    virtual State::StateResult Init_EventHandler(State::StateEvent* se);


 	/** Callback invocada al recibir una actualización de un topic local al que está suscrito
      *  @param topic Identificador del topic
      *  @param msg Mensaje recibido
      *  @param msg_len Tamaño del mensaje
      */
    virtual void subscriptionCb(const char* topic, void* msg, uint16_t msg_len);


 	/** Callback invocada al finalizar una publicación local
      *  @param topic Identificador del topic
      *  @param result Resultado de la publicación
      */
    virtual void publicationCb(const char* topic, int32_t result);


   	/** Chequea la integridad de los datos de configuración <_cfg>. En caso de que algo no sea
   	 * 	coherente, restaura a los valores por defecto y graba en memoria NV.
   	 * 	@return True si la integridad es correcta, False si es incorrecta
	 */
	virtual bool checkIntegrity();


   	/** Establece la configuración por defecto grabándola en memoria NV
	 */
	virtual void setDefaultConfig();


   	/** Recupera la configuración de memoria NV
	 */
	virtual void restoreConfig();


   	/** Graba la configuración en memoria NV
	 */
	virtual void saveConfig();


	/** Graba un parámetro en la memoria NV
	 * 	@param param_id Identificador del parámetro
	 * 	@param data Datos asociados
	 * 	@param size Tamaño de los datos
	 * 	@param type Tipo de los datos
	 * 	@return True: éxito, False: no se pudo recuperar
	 */
	virtual bool saveParameter(const char* param_id, void* data, size_t size, NVSInterface::KeyValueType type){
		return ActiveModule::saveParameter(param_id, data, size, type);
	}


	/** Recupera un parámetro de la memoria NV
	 * 	@param param_id Identificador del parámetro
	 * 	@param data Receptor de los datos asociados
	 * 	@param size Tamaño de los datos a recibir
	 * 	@param type Tipo de los datos
	 * 	@return True: éxito, False: no se pudo recuperar
	 */
	virtual bool restoreParameter(const char* param_id, void* data, size_t size, NVSInterface::KeyValueType type){
		return ActiveModule::restoreParameter(param_id, data, size, type);
	}


	/** Actualiza el estado de activación de la luminaria. Internamente se actualiza '_stat'
	 *
	 * @param value Nuevo valor de activación: 0=Off, 1..99=Dim, 100=On
	 */
	void updateLightValue(uint8_t value);


	/** Ejecuta el simulador de eventos
	 *
	 */
	void eventSimulatorCb();


	/** Actualiza el estado de la salida y notifica
	 *
	 * @param value Nuevo estado de la salida
	 */
	void _updateAndNotify(uint8_t value);


	/** Aplica la curva de activación en la carga
	 *
	 * @param value Valor a aplicar
	 * @return Valor resultante tras la curva
	 */
	uint8_t _applyCurve(uint8_t value);


	/** Actualiza la configuración
	 *
	 * @param data Nueva configuración a aplicar
	 * @param err Recibe los errores generados durante la actualización
	 */
	void _updateConfig(const light_manager& data, Blob::ErrorData_t& err);

};
     
#endif /*__LightManager__H */

/**** END OF FILE ****/


