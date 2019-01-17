/*
 * LightManager.cpp
 *
 *  Created on: Ene 2018
 *      Author: raulMrello
 */

#include "LightManager.h"

//------------------------------------------------------------------------------------
//-- PRIVATE TYPEDEFS ----------------------------------------------------------------
//------------------------------------------------------------------------------------

/** Macro para imprimir trazas de depuración, siempre que se haya configurado un objeto
 *	Logger válido (ej: _debug)
 */
static const char* _MODULE_ = "[LightM]........";
#define _EXPR_	(_defdbg && !IS_ISR())

 
//------------------------------------------------------------------------------------
//-- PUBLIC METHODS IMPLEMENTATION ---------------------------------------------------
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------
LightManager::LightManager(PinName pin010, FSManager* fs, bool defdbg) : ActiveModule("LightM", osPriorityNormal, 3096, fs, defdbg) {

	if(pin010 == NC){
		_driver010 = NULL;
	}
	else{
		_driver010 = new Driver_Pwm010(pin010, Driver_Pwm010::OnIsLowLevel, 1, defdbg);
		MBED_ASSERT(_driver010);
		_driver010->setScaleFactor(1);
	}

	// crea el scheduler
	_sched = new Scheduler(Blob::MaxAllowedActionDataInArray, _lightdata.cfg.outData.actions, fs, defdbg);
	MBED_ASSERT(_sched);

	// Carga callbacks estáticas de publicación/suscripción
    _publicationCb = callback(this, &LightManager::publicationCb);

    _sim_tmr = new RtosTimer(callback(this, &LightManager::eventSimulatorCb), osTimerPeriodic, "LightSimTmr");
}


//------------------------------------------------------------------------------------
void LightManager::startSimulator() {
	_sim_counter = 0;
	_sim_tmr->start(1000);
}


//------------------------------------------------------------------------------------
void LightManager::stopSimulator() {
	_sim_tmr->stop();
}


//------------------------------------------------------------------------------------
osStatus LightManager::putMessage(State::Msg *msg){
    osStatus ost = _queue.put(msg, ActiveModule::DefaultPutTimeout);
    if(ost != osOK){
        DEBUG_TRACE_I(_EXPR_, _MODULE_, "QUEUE_PUT_ERROR %d", ost);
    }
    return ost;
}



//------------------------------------------------------------------------------------
//-- PROTECTED METHODS IMPLEMENTATION ------------------------------------------------
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------
void LightManager::subscriptionCb(const char* topic, void* msg, uint16_t msg_len){

    // si es un comando para actualizar los parámetros minmax...
    if(MQ::MQClient::isTokenRoot(topic, "set/cfg")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);


        // Antes de nada, chequea que el tamaño de la zona horaria es correcto, en caso contrario, descarta el topic
        if(msg_len != sizeof(Blob::SetRequest_t<Blob::LightCfgData_t>)){
        	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_MSG. Error en el nº de datos del mensaje, topic [%s]", topic);
			return;
        }

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        // el mensaje es un blob tipo Blob::LightCfgData_t
        Blob::SetRequest_t<Blob::LightCfgData_t>* req = (Blob::SetRequest_t<Blob::LightCfgData_t>*)Heap::memAlloc(sizeof(Blob::SetRequest_t<Blob::LightCfgData_t>));
        MBED_ASSERT(req);
        *req = *((Blob::SetRequest_t<Blob::LightCfgData_t>*)msg);
        op->sig = RecvCfgSet;
		// apunta a los datos
		op->msg = req;

		// postea en la cola de la máquina de estados
		if(putMessage(op) != osOK){
			if(op->msg){
				Heap::memFree(op->msg);
			}
			Heap::memFree(op);
		}
        return;
    }

    // si es un comando para actualizar el estado de la luminaria
    if(MQ::MQClient::isTokenRoot(topic, "set/value")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        // Antes de nada, chequea que el tamaño de la zona horaria es correcto, en caso contrario, descarta el topic
        if(msg_len != sizeof(Blob::SetRequest_t<Blob::LightStatData_t>)){
        	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_MSG. Error en el nº de datos del mensaje, topic [%s]", topic);
			return;
        }

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        // el mensaje es un blob tipo Blob::LightCfgData_t
        Blob::SetRequest_t<Blob::LightStatData_t>* req = (Blob::SetRequest_t<Blob::LightStatData_t>*)Heap::memAlloc(sizeof(Blob::SetRequest_t<Blob::LightStatData_t>));
        MBED_ASSERT(req);
        *req = *((Blob::SetRequest_t<Blob::LightStatData_t>*)msg);
        op->sig = RecvStatSet;
		// apunta a los datos
		op->msg = req;

		// postea en la cola de la máquina de estados
		if(putMessage(op) != osOK){
			if(op->msg){
				Heap::memFree(op->msg);
			}
			Heap::memFree(op);
		}
        return;
    }

    // si es un comando para notificar un cambio en el luxómetro
    if(MQ::MQClient::isTokenRoot(topic, "set/lux")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        // Antes de nada, chequea que el tamaño corresponde con un valor de lux
        if(msg_len != sizeof(Blob::LightLuxLevel)){
        	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_MSG. Error en el nº de datos del mensaje, topic [%s]", topic);
			return;
        }

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        // el mensaje es un blob tipo Blob::GetRequest_t
        Blob::LightLuxLevel* ldata = (Blob::LightLuxLevel*)Heap::memAlloc(sizeof(Blob::LightLuxLevel));
		MBED_ASSERT(ldata);
		*ldata = *((Blob::LightLuxLevel*)msg);
		op->sig = RecvLuxSet;
		// apunta a los datos
		op->msg = ldata;

		// postea en la cola de la máquina de estados
		if(putMessage(op) != osOK){
			if(op->msg){
				Heap::memFree(op->msg);
			}
			Heap::memFree(op);
		}
        return;
    }

    // si es un comando para notificar un cambio en el timestamp del calendario
    if(MQ::MQClient::isTokenRoot(topic, "set/time")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        // Antes de nada, chequea que el tamaño corresponde con un valor de lux
        if(msg_len != sizeof(Blob::AstCalStatData_t)){
        	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_MSG. Error en el nº de datos del mensaje, topic [%s]", topic);
			return;
        }

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        Blob::AstCalStatData_t* tdata = (Blob::AstCalStatData_t*)Heap::memAlloc(sizeof(Blob::AstCalStatData_t));
		MBED_ASSERT(tdata);
		*tdata = *((Blob::AstCalStatData_t*)msg);
		op->sig = RecvTimeSet;
		// apunta a los datos
		op->msg = tdata;

		// postea en la cola de la máquina de estados
		if(putMessage(op) != osOK){
			if(op->msg){
				Heap::memFree(op->msg);
			}
			Heap::memFree(op);
		}
        return;
    }


    // si es un comando para solicitar los parámetros minmax...
    if(MQ::MQClient::isTokenRoot(topic, "get/cfg")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        // el mensaje es un blob tipo Blob::GetRequest_t
		Blob::GetRequest_t* req = (Blob::GetRequest_t*)Heap::memAlloc(sizeof(Blob::GetRequest_t));
		MBED_ASSERT(req);
		*req = *((Blob::GetRequest_t*)msg);
		op->sig = RecvCfgGet;
		// apunta a los datos
		op->msg = req;

		// postea en la cola de la máquina de estados
		if(putMessage(op) != osOK){
			if(op->msg){
				Heap::memFree(op->msg);
			}
			Heap::memFree(op);
		}
        return;
    }

    // si es un comando para solicitar el estado.
    if(MQ::MQClient::isTokenRoot(topic, "get/value")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        // el mensaje es un blob tipo Blob::GetRequest_t
		Blob::GetRequest_t* req = (Blob::GetRequest_t*)Heap::memAlloc(sizeof(Blob::GetRequest_t));
		MBED_ASSERT(req);
		*req = *((Blob::GetRequest_t*)msg);
		op->sig = RecvStatGet;
		// apunta a los datos
		op->msg = req;

		// postea en la cola de la máquina de estados
		if(putMessage(op) != osOK){
			if(op->msg){
				Heap::memFree(op->msg);
			}
			Heap::memFree(op);
		}
        return;
    }

    // si es un comando para solicitar los parámetros minmax...
    if(MQ::MQClient::isTokenRoot(topic, "get/boot")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        op->sig = RecvBootGet;
		// apunta a los datos
		op->msg = NULL;

		// postea en la cola de la máquina de estados
		if(putMessage(op) != osOK){
			if(op->msg){
				Heap::memFree(op->msg);
			}
			Heap::memFree(op);
		}
        return;
    }

    DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_TOPIC. No se puede procesar el topic [%s]", topic);
}


//------------------------------------------------------------------------------------
State::StateResult LightManager::Init_EventHandler(State::StateEvent* se){
	State::Msg* st_msg = (State::Msg*)se->oe->value.p;
    switch((int)se->evt){
        case State::EV_ENTRY:{
        	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Iniciando recuperación de datos...");
        	// recupera los datos de memoria NV
        	restoreConfig();

        	// realiza la suscripción local ej: "cmd/$module/#"
        	char* sub_topic_local = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
        	MBED_ASSERT(sub_topic_local);
        	sprintf(sub_topic_local, "set/+/%s", _sub_topic_base);
        	if(MQ::MQClient::subscribe(sub_topic_local, new MQ::SubscribeCallback(this, &LightManager::subscriptionCb)) == MQ::SUCCESS){
        		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Sucripción LOCAL hecha a %s", sub_topic_local);
        	}
        	else{
        		DEBUG_TRACE_E(_EXPR_, _MODULE_, "ERR_SUBSC en la suscripción LOCAL a %s", sub_topic_local);
        	}
        	sprintf(sub_topic_local, "get/+/%s", _sub_topic_base);
        	if(MQ::MQClient::subscribe(sub_topic_local, new MQ::SubscribeCallback(this, &LightManager::subscriptionCb)) == MQ::SUCCESS){
        		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Sucripción LOCAL hecha a %s", sub_topic_local);
        	}
        	else{
        		DEBUG_TRACE_E(_EXPR_, _MODULE_, "ERR_SUBSC en la suscripción LOCAL a %s", sub_topic_local);
        	}
        	Heap::memFree(sub_topic_local);
            return State::HANDLED;
        }

        case State::EV_TIMED:{
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicación en cmd/$BASE/cfg/set
        case RecvCfgSet:{
        	Blob::SetRequest_t<Blob::LightCfgData_t>* req = (Blob::SetRequest_t<Blob::LightCfgData_t>*)st_msg->msg;
			// si no hay errores, actualiza la configuración
			if(req->_error.code == Blob::ErrOK){
				_updateConfig(req->data, req->_error);
			}
        	// si hay errores en el mensaje o en la actualización, devuelve resultado sin hacer nada
        	if(req->_error.code != Blob::ErrOK){
        		char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
				MBED_ASSERT(pub_topic);
				sprintf(pub_topic, "stat/cfg/%s", _pub_topic_base);

				Blob::Response_t<Blob::LightCfgData_t>* resp = new Blob::Response_t<Blob::LightCfgData_t>(req->idTrans, req->_error, _lightdata.cfg);
				MQ::MQClient::publish(pub_topic, resp, sizeof(Blob::Response_t<Blob::LightCfgData_t>), &_publicationCb);
				delete(resp);
				Heap::memFree(pub_topic);
				return State::HANDLED;
        	}

        	// almacena en el sistema de ficheros
        	saveConfig();

        	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Config actualizada");

        	// si está habilitada la notificación de actualización, lo notifica
        	if((_lightdata.cfg.updFlagMask & Blob::EnableLightCfgUpdNotif) != 0){
				char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
				MBED_ASSERT(pub_topic);
				sprintf(pub_topic, "stat/cfg/%s", _pub_topic_base);
				Blob::Response_t<Blob::LightCfgData_t>* resp = new Blob::Response_t<Blob::LightCfgData_t>(req->idTrans, req->_error, _lightdata.cfg);
				MQ::MQClient::publish(pub_topic, resp, sizeof(Blob::Response_t<Blob::LightCfgData_t>), &_publicationCb);
				delete(resp);
				Heap::memFree(pub_topic);
        	}

            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicación en cmd/$BASE/value/set
        case RecvStatSet:{
        	Blob::SetRequest_t<Blob::LightStatData_t>* req = (Blob::SetRequest_t<Blob::LightStatData_t>*)st_msg->msg;
        	// si el mensaje recibido no tiene errores de preprocesado continúa
        	if(req->_error.code == Blob::ErrOK){
				// actualiza el estado de la luminaria
				uint8_t value =  req->data.outValue;
				// si la salida está fuera de rango, no hace nada y devuelve un error
				if(value > Blob::LightActionOutMax){
					req->_error.code = Blob::ErrRangeValue;
					strcpy(req->_error.descr, Blob::errList[req->_error.code]);
				}
				else{
					// actualizo eventos de cambio de estado
					// cambio a Off
					if(value == 0){
						_lightdata.stat.flags = Blob::LightOutOffEvt;
					}
					// cambio de estado a On
					else if(value == 100){
						_lightdata.stat.flags = Blob::LightOutOnEvt;
					}
					// cambio de estado de regulación
					else{
						_lightdata.stat.flags = Blob::LightOutLevelChangeEvt;
					}
					_lightdata.stat.outValue = value;
					if(_driver010){
						_driver010->setLevel(_applyCurve(_lightdata.stat.outValue));
					}

					DEBUG_TRACE_D(_EXPR_, _MODULE_, "LIGHT_VALUE_SET, actualizado = %d", _lightdata.stat.outValue);
				}
        	}

        	// notifica el cambio de estado
        	char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
			MBED_ASSERT(pub_topic);
			sprintf(pub_topic, "stat/value/%s", _pub_topic_base);
			Blob::Response_t<Blob::LightStatData_t>* resp = new Blob::Response_t<Blob::LightStatData_t>(req->idTrans, req->_error, _lightdata.stat);
			MQ::MQClient::publish(pub_topic, resp, sizeof(Blob::Response_t<Blob::LightStatData_t>), &_publicationCb);
			delete(resp);
			Heap::memFree(pub_topic);
        	return State::HANDLED;
        }

        // Procesa datos recibidos de la publicación en cmd/$BASE/cfg/get
        case RecvCfgGet:{
        	Blob::GetRequest_t* req = (Blob::GetRequest_t*)st_msg->msg;

			DEBUG_TRACE_I(_EXPR_, _MODULE_, "Respondiendo datos de configuración solicitados");
			char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
			MBED_ASSERT(pub_topic);
			sprintf(pub_topic, "stat/cfg/%s", _pub_topic_base);

			// responde con los datos solicitados y con los errores (si hubiera) de la decodificación de la solicitud
			Blob::Response_t<Blob::LightCfgData_t>* resp = new Blob::Response_t<Blob::LightCfgData_t>(req->idTrans, req->_error, _lightdata.cfg);
			MQ::MQClient::publish(pub_topic, resp, sizeof(Blob::Response_t<Blob::LightCfgData_t>), &_publicationCb);
			delete(resp);

        	// libera la memoria asignada al topic de publicación
			Heap::memFree(pub_topic);
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicación en cmd/$BASE/value/get
        case RecvStatGet:{
        	Blob::GetRequest_t* req = (Blob::GetRequest_t*)st_msg->msg;
        	// prepara el topic al que responder
        	char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
			MBED_ASSERT(pub_topic);
			sprintf(pub_topic, "stat/value/%s", _pub_topic_base);

			DEBUG_TRACE_I(_EXPR_, _MODULE_, "Respondiendo datos de estado solicitados");

			// responde con los datos solicitados y con los errores (si hubiera) de la decodificación de la solicitud
			Blob::Response_t<Blob::LightStatData_t>* resp = new Blob::Response_t<Blob::LightStatData_t>(req->idTrans, req->_error, _lightdata.stat);
			// borra los flags de evento ya que es una respuesta sin más
			resp->data.flags = Blob::LightNoEvents;
			MQ::MQClient::publish(pub_topic, resp, sizeof(Blob::Response_t<Blob::LightStatData_t>), &_publicationCb);
			delete(resp);

        	// libera la memoria asignada al topic de publicación
			Heap::memFree(pub_topic);
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicación en get/boot
        case RecvBootGet:{
        	char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
			MBED_ASSERT(pub_topic);
			sprintf(pub_topic, "stat/boot/%s", _pub_topic_base);
			MQ::MQClient::publish(pub_topic, &_lightdata, sizeof(Blob::LightBootData_t), &_publicationCb);
			Heap::memFree(pub_topic);

            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicación en set/time
        case RecvTimeSet:{
        	Blob::AstCalStatData_t ast = *(Blob::AstCalStatData_t*)st_msg->msg;
			int8_t new_out_value;
			// ejecuta el scheduler y en caso de que haya un nuevo estado de la carga, lo notifica
			if((new_out_value = _sched->updateTimestamp(ast)) != -1){
				_updateAndNotify(new_out_value);
			}
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicación en set/lux
        case RecvLuxSet:{
	        Blob::LightLuxLevel lux = *(Blob::LightLuxLevel*)st_msg->msg;
	        int8_t new_out_value;
			// ejecuta el scheduler y en caso de que haya un nuevo estado de la carga, lo notifica
			if((new_out_value = _sched->updateLux(lux)) != -1){
				_updateAndNotify(new_out_value);
			}
            return State::HANDLED;
        }

        case State::EV_EXIT:{
            nextState();
            return State::HANDLED;
        }

        default:{
        	return State::IGNORED;
        }

     }
}


//------------------------------------------------------------------------------------
osEvent LightManager:: getOsEvent(){
	return _queue.get();
}



//------------------------------------------------------------------------------------
void LightManager::publicationCb(const char* topic, int32_t result){

}


//------------------------------------------------------------------------------------
bool LightManager::checkIntegrity(){
	#warning TODO
	DEBUG_TRACE_W(_EXPR_, _MODULE_, "~~~~ TODO ~~~~ LightManager::checkIntegrity");
	// Hacer lo que corresponda
	// ...

	// chequeo de integridad de acciones
	if(!_sched->checkIntegrity())
		return false;
	return true;
}


//------------------------------------------------------------------------------------
void LightManager::setDefaultConfig(){
	// inicializo flags
	_lightdata.cfg.updFlagMask = Blob::EnableLightCfgUpdNotif;
	_lightdata.cfg.evtFlagMask = (Blob::LightEvtFlags)(Blob::LightOutOnEvt | Blob::LightOutOffEvt | Blob::LightOutLevelChangeEvt);
	// borra los rangos del sensor de iluminación
	_lightdata.cfg.alsData = {0, 0, 0};
	// establece control con relé NA y PWM0_10
	_lightdata.cfg.outData.mode = (Blob::LightOutModeFlags)(Blob::LightOutRelayNA | Blob::LightOutPwm);
	// establece una curva de actuación lineal
	_lightdata.cfg.outData.curve.samples = 3;
	_lightdata.cfg.outData.curve.data[0] = 0;
	_lightdata.cfg.outData.curve.data[1] = 50;
	_lightdata.cfg.outData.curve.data[2] = 100;
	// borra todos los programas
	_sched->clrActions();
	_lightdata.cfg.outData.numActions = _sched->getActionCount();

	saveConfig();
}


//------------------------------------------------------------------------------------
void LightManager::restoreConfig(){
	uint32_t crc = 0;

	// inicializa estado como apagado
	_lightdata.stat.outValue = 0;
	_lightdata.stat.flags = Blob::LightNoEvents;
	if(_driver010){
		_driver010->setLevel(_lightdata.stat.outValue);
	}

	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recuperando datos de memoria NV...");
	bool success = true;
	if(!restoreParameter("LigUpdFla", &_lightdata.cfg.updFlagMask, sizeof(uint32_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo UpdFlags!");
		success = false;
	}
	if(!restoreParameter("LigEvtFla", &_lightdata.cfg.evtFlagMask, sizeof(uint32_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo EvtFlags!");
		success = false;
	}
	if(!restoreParameter("LigAlsDat", &_lightdata.cfg.alsData, sizeof(Blob::LightAlsData_t), NVSInterface::TypeBlob)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo AlsData!");
		success = false;
	}
	if(!restoreParameter("LigOutDatMod", &_lightdata.cfg.outData.mode, sizeof(Blob::LightOutModeFlags), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo OutDataMode!");
		success = false;
	}
	if(!restoreParameter("LigOutDatCur", &_lightdata.cfg.outData.curve, sizeof(Blob::LightCurve_t), NVSInterface::TypeBlob)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo OutDataCurve!");
		success = false;
	}
	if(!restoreParameter("LigOutDatNum", &_lightdata.cfg.outData.numActions, sizeof(uint8_t), NVSInterface::TypeUint8)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo OutDataNumActions!");
		success = false;
	}
	if(!_sched->restoreActionList()){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo OutDataActions!");
		success = false;
	}

	if(!restoreParameter("LigChk", &crc, sizeof(uint32_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo Checksum!");
		success = false;
	}
	// borra los parámetros no recuperados
	_lightdata.cfg.keys = Blob::LightKeyNone;

	if(success){
		// chequea el checksum crc32 y después la integridad de los datos
		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Datos recuperados. Chequeando integridad...");
		if(Blob::getCRC32(&_lightdata.cfg, sizeof(Blob::LightCfgData_t)) != crc){
			DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_CFG. Ha fallado el checksum");
		}
    	else if(!checkIntegrity()){
    		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_CFG. Ha fallado el check de integridad.");
    	}
    	else{
    		DEBUG_TRACE_W(_EXPR_, _MODULE_, "Check de integridad OK!");
    		return;
    	}
	}
	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_FS. Error en la recuperación de datos. Establece configuración por defecto");
	setDefaultConfig();
}


//------------------------------------------------------------------------------------
void LightManager::saveConfig(){
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Guardando datos en memoria NV...");
	// almacena en el sistema de ficheros
	if(!saveParameter("LigUpdFla", &_lightdata.cfg.updFlagMask, sizeof(uint32_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS grabando UpdFlags!");
	}
	if(!saveParameter("LigEvtFla", &_lightdata.cfg.evtFlagMask, sizeof(uint32_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS grabando EvtFlags!");
	}
	if(!saveParameter("LigAlsDat", &_lightdata.cfg.alsData, sizeof(Blob::LightAlsData_t), NVSInterface::TypeBlob)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS grabando AlsData!");
	}
	if(!saveParameter("LigOutDatMod", &_lightdata.cfg.outData.mode, sizeof(Blob::LightOutModeFlags), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS grabando OutDataMode!");
	}
	if(!saveParameter("LigOutDatCur", &_lightdata.cfg.outData.curve, sizeof(Blob::LightCurve_t), NVSInterface::TypeBlob)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS grabando OutDataMode!");
	}
	if(!saveParameter("LigOutDatNum", &_lightdata.cfg.outData.numActions, sizeof(uint8_t), NVSInterface::TypeUint8)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS grabando OutDataNumActions!");
	}
	if(!_sched->saveActionList()){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS grabando OutDataActions!");
	}

	// borra los parámetros no recuperados
	_lightdata.cfg.keys = Blob::LightKeyNone;
	uint32_t crc = Blob::getCRC32(&_lightdata.cfg, sizeof(Blob::LightCfgData_t));
	if(!saveParameter("LigChk", &crc, sizeof(uint32_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS grabando Checksum!");
	}
}


//------------------------------------------------------------------------------------
void LightManager::eventSimulatorCb() {
	_sim_counter ++;
	if((_sim_counter % 30) == 0){
		_lightdata.stat.flags = (uint32_t)Blob::LightOutLevelChangeEvt;
		_lightdata.stat.outValue = (int8_t)(rand() % 100);
		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Simulando evento %x, outValue=%d, contador = %d", _lightdata.stat.flags, _lightdata.stat.outValue, _sim_counter);
		char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
		MBED_ASSERT(pub_topic);
		sprintf(pub_topic, "stat/value/%s", _pub_topic_base);
		MQ::MQClient::publish(pub_topic, &_lightdata.stat, sizeof(Blob::LightStatData_t), &_publicationCb);
		Heap::memFree(pub_topic);
	}
}


// ----------------------------------------------------------------------------------
void LightManager::_updateConfig(const Blob::LightCfgData_t& cfg, Blob::ErrorData_t& err){
	if(cfg.keys == Blob::LightKeyNone){
		err.code = Blob::ErrEmptyContent;
		goto _updateConfigExit;
	}

	if(cfg.keys & Blob::LightKeyCfgUpd){
		_lightdata.cfg.updFlagMask = cfg.updFlagMask;
	}
	if(cfg.keys & Blob::LightKeyCfgEvt){
		_lightdata.cfg.evtFlagMask = cfg.evtFlagMask;
	}
	if(cfg.keys & Blob::LightKeyCfgAls){
		_lightdata.cfg.alsData = cfg.alsData;
	}
	if(cfg.keys & Blob::LightKeyCfgOutm){
		_lightdata.cfg.outData.mode = cfg.outData.mode;
	}
	if(cfg.keys & Blob::LightKeyCfgCurve){
		_lightdata.cfg.outData.curve = cfg.outData.curve;
	}
	if(cfg.keys & Blob::LightKeyCfgActs){
		_lightdata.cfg.outData.numActions = cfg.outData.numActions;
		for(int i=0;i<cfg.outData.numActions;i++)
			_lightdata.cfg.outData.actions[cfg.outData.actions[i].id] = cfg.outData.actions[i];
	}
_updateConfigExit:
	strcpy(err.descr, Blob::errList[err.code]);
}


// ----------------------------------------------------------------------------------
void LightManager::_updateAndNotify(uint8_t value){
	// si el estado es el mismo, no hace nada
	if(value == _lightdata.stat.outValue){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "Nuevo estado ya establecido.");
		return;
	}

	// si la salida está fuera de rango, fija valor máximo
	if(value > Blob::LightActionOutMax){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "Nuevo estado fuera de rango, val=%d. Ajustado a max.", value);
		value = Blob::LightActionOutMax;
	}

	// actualizo eventos de cambio de estado
	// cambio a Off
	if(value == 0){
		_lightdata.stat.flags = Blob::LightOutOffEvt;
	}
	// cambio de estado a On
	else if(value == 100){
		_lightdata.stat.flags = Blob::LightOutOnEvt;
	}
	// cambio de estado de regulación
	else{
		_lightdata.stat.flags = Blob::LightOutLevelChangeEvt;
	}
	_lightdata.stat.outValue = value;
	if(_driver010){
		_driver010->setLevel(_applyCurve(_lightdata.stat.outValue));
	}
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "LIGHT_VALUE, actualizado = %d", _lightdata.stat.outValue);

	// notifica el cambio de estado
	char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
	MBED_ASSERT(pub_topic);
	sprintf(pub_topic, "stat/value/%s", _pub_topic_base);
	MQ::MQClient::publish(pub_topic, &_lightdata.stat, sizeof(Blob::LightStatData_t), &_publicationCb);
	Heap::memFree(pub_topic);
}


// ----------------------------------------------------------------------------------
uint8_t LightManager::_applyCurve(uint8_t value){
	// si no hay curva, devuelve el mismo valor
	if(_lightdata.cfg.outData.curve.samples != Blob::LightCurveSampleCount)
		return value;

	// si el último punto de la curva, lo devuelve
	int i = value/10;
	if(i == Blob::LightCurveSampleCount - 1)
		return _lightdata.cfg.outData.curve.data[i];

	// si es un punto intermedio, aplica la curva
	uint8_t p1 = _lightdata.cfg.outData.curve.data[i];
	uint8_t p2 = _lightdata.cfg.outData.curve.data[i+1];
	uint16_t pos = value%10;
	uint8_t result = (pos*(p2-p1))/10 + p1;
	return result;
}
