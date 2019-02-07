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

	// Establece el soporte de JSON
	_json_supported = false;
	#if LIGHTMANAGER_ENABLE_JSON_SUPPORT == 1
	_json_supported = true;
	#endif


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

        Blob::SetRequest_t<Blob::LightCfgData_t>* req = NULL;
        bool json_decoded = false;
		if(_json_supported){
			req = (Blob::SetRequest_t<Blob::LightCfgData_t>*)Heap::memAlloc(sizeof(Blob::SetRequest_t<Blob::LightCfgData_t>));
			MBED_ASSERT(req);
			if(!(json_decoded = decodeSetRequest(*req, (char*)msg))){
				Heap::memFree(req);
				DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_JSON. Decodificando el mensaje");
			}
		}

        // en primer lugar asegura que los datos tienen el tamaño correcto
        if(!json_decoded && msg_len != sizeof(Blob::SetRequest_t<Blob::LightCfgData_t>)){
        	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_MSG. Error en el nº de datos del mensaje, topic [%s]", topic);
			return;
        }

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        if(!json_decoded){
			// el mensaje es un blob tipo Blob::AMCfgData_t
			req = (Blob::SetRequest_t<Blob::LightCfgData_t>*)Heap::memAlloc(sizeof(Blob::SetRequest_t<Blob::LightCfgData_t>));
			MBED_ASSERT(req);
			*req = *((Blob::SetRequest_t<Blob::LightCfgData_t>*)msg);
        }
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

        Blob::SetRequest_t<Blob::LightStatData_t>* req = NULL;
        bool json_decoded = false;
		if(_json_supported){
			req = (Blob::SetRequest_t<Blob::LightStatData_t>*)Heap::memAlloc(sizeof(Blob::SetRequest_t<Blob::LightStatData_t>));
			MBED_ASSERT(req);
			if(!(json_decoded = _decodeSetReqValue(*req, (char*)msg))){
				Heap::memFree(req);
				DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_JSON. Decodificando el mensaje");
			}
		}

        // en primer lugar asegura que los datos tienen el tamaño correcto
        if(!json_decoded && msg_len != sizeof(Blob::SetRequest_t<Blob::LightStatData_t>)){
        	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_MSG. Error en el nº de datos del mensaje, topic [%s]", topic);
			return;
        }

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        if(!json_decoded){
			// el mensaje es un blob tipo Blob::LightStatData_t
			req = (Blob::SetRequest_t<Blob::LightStatData_t>*)Heap::memAlloc(sizeof(Blob::SetRequest_t<Blob::LightStatData_t>));
			MBED_ASSERT(req);
			*req = *((Blob::SetRequest_t<Blob::LightStatData_t>*)msg);
        }
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

        Blob::LightLuxLevel* req = NULL;
        bool json_decoded = false;
		if(_json_supported){
			req = (Blob::LightLuxLevel*)Heap::memAlloc(sizeof(Blob::LightLuxLevel));
			MBED_ASSERT(req);
			if(!(json_decoded = _decodeSetLux(*req, (char*)msg))){
				Heap::memFree(req);
				DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_JSON. Decodificando el mensaje");
			}
		}

        // en primer lugar asegura que los datos tienen el tamaño correcto
        if(!json_decoded && msg_len != sizeof(Blob::LightLuxLevel)){
        	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_MSG. Error en el nº de datos del mensaje, topic [%s]", topic);
			return;
        }

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        if(!json_decoded){
			// el mensaje es un blob tipo Blob::LightStatData_t
			req = (Blob::LightLuxLevel*)Heap::memAlloc(sizeof(Blob::LightLuxLevel));
			MBED_ASSERT(req);
			*req = *((Blob::LightLuxLevel*)msg);
        }
		op->sig = RecvLuxSet;
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

    // si es un comando para notificar un cambio en el timestamp del calendario
    if(MQ::MQClient::isTokenRoot(topic, "set/time")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        Blob::AstCalStatData_t *req = NULL;
        bool json_decoded = false;
		if(_json_supported){
			req = (Blob::AstCalStatData_t*)Heap::memAlloc(sizeof(Blob::AstCalStatData_t));
			MBED_ASSERT(req);
			if(!(json_decoded = _decodeSetTime(*req, (char*)msg))){
				Heap::memFree(req);
				DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_JSON. Decodificando el mensaje");
			}
		}

        // en primer lugar asegura que los datos tienen el tamaño correcto
        if(!json_decoded && msg_len != sizeof(Blob::AstCalStatData_t)){
        	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_MSG. Error en el nº de datos del mensaje, topic [%s]", topic);
			return;
        }

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        if(!json_decoded){
			// el mensaje es un blob tipo Blob::LightStatData_t
			req = (Blob::AstCalStatData_t*)Heap::memAlloc(sizeof(Blob::AstCalStatData_t));
			MBED_ASSERT(req);
			*req = *((Blob::AstCalStatData_t*)msg);
        }
		op->sig = RecvTimeSet;
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

    // si es un comando para solicitar lectura de datos...
    if(MQ::MQClient::isTokenRoot(topic, "get/cfg") || MQ::MQClient::isTokenRoot(topic, "get/value")){
        DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recibido topic %s", topic);

        Blob::GetRequest_t* req = NULL;
        bool json_decoded = false;
        if(_json_supported){
			req = (Blob::GetRequest_t*)Heap::memAlloc(sizeof(Blob::GetRequest_t));
			MBED_ASSERT(req);
			if(!(json_decoded = JSON::decodeGetRequest(*req, (char*)msg))){
				Heap::memFree(req);
			}
        }

        // Antes de nada, chequea que el tamaño de la zona horaria es correcto, en caso contrario, descarta el topic
        if(!json_decoded && msg_len != sizeof(Blob::GetRequest_t)){
        	DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_MSG. Error en el nº de datos del mensaje, topic [%s]", topic);
			return;
        }

        // crea el mensaje para publicar en la máquina de estados
        State::Msg* op = (State::Msg*)Heap::memAlloc(sizeof(State::Msg));
        MBED_ASSERT(op);

        // el mensaje es un blob tipo Blob::GetRequest_t
        if(!json_decoded){
        	req = (Blob::GetRequest_t*)Heap::memAlloc(sizeof(Blob::GetRequest_t));
        	MBED_ASSERT(req);
        	*req = *((Blob::GetRequest_t*)msg);
        }
        // por defecto supongo get/cfg
        op->sig = RecvCfgGet;
        if(MQ::MQClient::isTokenRoot(topic, "get/value"))
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

    // si es un comando para solicitar la configuración
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
				_updateConfig(req->data, req->keys, req->_error);
			}
        	// si hay errores en el mensaje o en la actualización, devuelve resultado sin hacer nada
        	if(req->_error.code != Blob::ErrOK){
        		char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
				MBED_ASSERT(pub_topic);
				sprintf(pub_topic, "stat/cfg/%s", _pub_topic_base);

				Blob::Response_t<Blob::LightCfgData_t>* resp = new Blob::Response_t<Blob::LightCfgData_t>(req->idTrans, req->_error, _lightdata.cfg);

				if(_json_supported){
					cJSON* jresp = encodeCfgResponse(*resp);
					if(jresp){
						char* jmsg = cJSON_Print(jresp);
						cJSON_Delete(jresp);
						MQ::MQClient::publish(pub_topic, jmsg, strlen(jmsg)+1, &_publicationCb);
						Heap::memFree(jmsg);
						delete(resp);
						Heap::memFree(pub_topic);
						return State::HANDLED;
					}
				}

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

				if(_json_supported){
					cJSON* jresp = encodeCfgResponse(*resp);
					if(jresp){
						char* jmsg = cJSON_Print(jresp);
						cJSON_Delete(jresp);
						MQ::MQClient::publish(pub_topic, jmsg, strlen(jmsg)+1, &_publicationCb);
						Heap::memFree(jmsg);
						delete(resp);
						Heap::memFree(pub_topic);
						return State::HANDLED;
					}
				}

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

			if(_json_supported){
				cJSON* jresp = encodeStatResponse(*resp);
				if(jresp){
					char* jmsg = cJSON_Print(jresp);
					cJSON_Delete(jresp);
					MQ::MQClient::publish(pub_topic, jmsg, strlen(jmsg)+1, &_publicationCb);
					Heap::memFree(jmsg);
					delete(resp);
					Heap::memFree(pub_topic);
					return State::HANDLED;
				}
			}

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

			if(_json_supported){
				cJSON* jresp = encodeCfgResponse(*resp);
				if(jresp){
					char* jmsg = cJSON_Print(jresp);
					cJSON_Delete(jresp);
					MQ::MQClient::publish(pub_topic, jmsg, strlen(jmsg)+1, &_publicationCb);
					Heap::memFree(jmsg);
					delete(resp);
					Heap::memFree(pub_topic);
					return State::HANDLED;
				}
			}

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

			if(_json_supported){
				cJSON* jresp = encodeStatResponse(*resp);
				if(jresp){
					char* jmsg = cJSON_Print(jresp);
					cJSON_Delete(jresp);
					MQ::MQClient::publish(pub_topic, jmsg, strlen(jmsg)+1, &_publicationCb);
					Heap::memFree(jmsg);
					delete(resp);
					Heap::memFree(pub_topic);
					return State::HANDLED;
				}
			}

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

			if(_json_supported){
				cJSON* jboot = _encodeBoot();
				if(jboot){
					char* jmsg = cJSON_Print(jboot);
					cJSON_Delete(jboot);
					MQ::MQClient::publish(pub_topic, jmsg, strlen(jmsg)+1, &_publicationCb);
					Heap::memFree(jmsg);
					Heap::memFree(pub_topic);
					return State::HANDLED;
				}
			}

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


// ----------------------------------------------------------------------------------
void LightManager::_updateConfig(const Blob::LightCfgData_t& cfg, uint32_t keys, Blob::ErrorData_t& err){
	if(keys == Blob::LightKeyNone){
		err.code = Blob::ErrEmptyContent;
		goto _updateConfigExit;
	}

	if(keys & Blob::LightKeyCfgUpd){
		_lightdata.cfg.updFlagMask = cfg.updFlagMask;
	}
	if(keys & Blob::LightKeyCfgEvt){
		_lightdata.cfg.evtFlagMask = cfg.evtFlagMask;
	}
	if(keys & Blob::LightKeyCfgAls){
		_lightdata.cfg.alsData = cfg.alsData;
	}
	if(keys & Blob::LightKeyCfgOutm){
		_lightdata.cfg.outData.mode = cfg.outData.mode;
	}
	if(keys & Blob::LightKeyCfgCurve){
		_lightdata.cfg.outData.curve = cfg.outData.curve;
	}
	if(keys & Blob::LightKeyCfgActs){
		_lightdata.cfg.outData.numActions = cfg.outData.numActions;
		for(int i=0;i<cfg.outData.numActions;i++)
			_lightdata.cfg.outData.actions[cfg.outData.actions[i].id] = cfg.outData.actions[i];
	}
_updateConfigExit:
	strcpy(err.descr, Blob::errList[err.code]);
}




//------------------------------------------------------------------------------------
//-- JSON SUPPORT --------------------------------------------------------------------
//------------------------------------------------------------------------------------


/** JSON keys */
static const char* p_actions		= "actions";
static const char* p_alsData		= "alsData";
static const char* p_astCorr 		= "astCorr";
static const char* p_astData 		= "astData";
static const char* p_cfg 			= "cfg";
static const char* p_code	 		= "code";
static const char* p_curve	 		= "curve";
static const char* p_data	 		= "data";
static const char* p_date 	 		= "date";
static const char* p_descr	 		= "descr";
static const char* p_error	 		= "error";
static const char* p_evtFlags 		= "evtFlags";
static const char* p_flags	 		= "flags";
static const char* p_header			= "header";
static const char* p_id  			= "id";
static const char* p_idTrans 		= "idTrans";
static const char* p_latitude 		= "latitude";
static const char* p_light			= "light";
static const char* p_longitude 		= "longitude";
static const char* p_lux			= "lux";
static const char* p_luxLevel		= "luxLevel";
static const char* p_max			= "max";
static const char* p_min			= "min";
static const char* p_mode			= "mode";
static const char* p_now			= "now";
static const char* p_numActions		= "numActions";
static const char* p_outData		= "outData";
static const char* p_outValue		= "outValue";
static const char* p_period			= "period";
static const char* p_reductionStart = "reductionStart";
static const char* p_reductionStop	= "reductionStop";
static const char* p_samples		= "samples";
static const char* p_stat	 		= "stat";
static const char* p_thres			= "thres";
static const char* p_time	 		= "time";
static const char* p_timestamp		= "timestamp";
static const char* p_updFlags 		= "updFlags";
static const char* p_wdowDawnStart 	= "wdowDawnStart";
static const char* p_wdowDawnStop 	= "wdowDawnStop";
static const char* p_wdowDuskStart 	= "wdowDuskStart";
static const char* p_wdowDuskStop 	= "wdowDuskStop";


//------------------------------------------------------------------------------------
cJSON* LightManager::encodeCfg(const Blob::LightCfgData_t& cfg){
	cJSON *alsData = NULL;
	cJSON *outData = NULL;
	cJSON *curve = NULL;
	cJSON *array = NULL;
	cJSON *value = NULL;
	cJSON *luxLevel = NULL;
	cJSON *light = NULL;

	if((light=cJSON_CreateObject()) == NULL){
		return NULL;
	}

	// key: light.updFlags
	cJSON_AddNumberToObject(light, p_updFlags, cfg.updFlagMask);

	// key: light.evtFlags
	cJSON_AddNumberToObject(light, p_evtFlags, cfg.evtFlagMask);

	// key: alsData
	if((alsData=cJSON_CreateObject()) == NULL){
		cJSON_Delete(light);
		return NULL;
	}

	// key: alsData.lux
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(alsData);
		cJSON_Delete(light);
		return NULL;
	}
	cJSON_AddNumberToObject(value, p_min, cfg.alsData.lux.min);
	cJSON_AddNumberToObject(value, p_max, cfg.alsData.lux.max);
	cJSON_AddNumberToObject(value, p_thres, cfg.alsData.lux.thres);
	cJSON_AddItemToObject(alsData, p_lux, value);
	cJSON_AddItemToObject(light, p_alsData, alsData);

	// key: outData
	if((outData=cJSON_CreateObject()) == NULL){
		cJSON_Delete(light);
		return NULL;
	}
	cJSON_AddNumberToObject(outData, p_mode, cfg.outData.mode);
	cJSON_AddNumberToObject(outData, p_numActions, cfg.outData.numActions);

	// key: outData.curve
	if((curve=cJSON_CreateObject()) == NULL){
		cJSON_Delete(outData);
		cJSON_Delete(light);
		return NULL;
	}
	cJSON_AddNumberToObject(curve, p_samples, cfg.outData.curve.samples);

	// key: outData.curve.data
	if((array=cJSON_CreateArray()) == NULL){
		cJSON_Delete(curve);
		cJSON_Delete(outData);
		cJSON_Delete(light);
		return NULL;
	}
	for(int i=0; i < Blob::LightCurveSampleCount; i++){
		if((value = cJSON_CreateNumber(cfg.outData.curve.data[i])) == NULL){
			cJSON_Delete(array);
			cJSON_Delete(curve);
			cJSON_Delete(outData);
			cJSON_Delete(light);
			return NULL;
		}
		cJSON_AddItemToArray(array, value);
	}
	cJSON_AddItemToObject(curve, p_data, array);
	cJSON_AddItemToObject(outData, p_curve, curve);

	// key: outData.actions
	if((array=cJSON_CreateArray()) == NULL){
		cJSON_Delete(outData);
		cJSON_Delete(light);
		return NULL;
	}
	for(int i=0; i < cfg.outData.numActions; i++){
		if((value = cJSON_CreateObject()) == NULL){
			cJSON_Delete(array);
			cJSON_Delete(outData);
			cJSON_Delete(light);
			return NULL;
		}
		cJSON_AddNumberToObject(value, p_id, cfg.outData.actions[i].id);
		cJSON_AddNumberToObject(value, p_flags, cfg.outData.actions[i].flags);
		cJSON_AddNumberToObject(value, p_date, cfg.outData.actions[i].date);
		cJSON_AddNumberToObject(value, p_time, cfg.outData.actions[i].time);
		cJSON_AddNumberToObject(value, p_astCorr, cfg.outData.actions[i].astCorr);
		cJSON_AddNumberToObject(value, p_outValue, cfg.outData.actions[i].outValue);
		if((luxLevel=cJSON_CreateObject()) == NULL){
			cJSON_Delete(value);
			cJSON_Delete(array);
			cJSON_Delete(outData);
			cJSON_Delete(light);
			return NULL;
		}
		cJSON_AddNumberToObject(luxLevel, p_min, cfg.outData.actions[i].luxLevel.min);
		cJSON_AddNumberToObject(luxLevel, p_max, cfg.outData.actions[i].luxLevel.max);
		cJSON_AddNumberToObject(luxLevel, p_thres, cfg.outData.actions[i].luxLevel.thres);
		cJSON_AddItemToObject(value, p_luxLevel, luxLevel);
		cJSON_AddItemToArray(array, value);
	}
	cJSON_AddItemToObject(outData, p_actions, array);
	cJSON_AddItemToObject(light, p_outData, outData);
	return light;
}


//------------------------------------------------------------------------------------
cJSON* LightManager::encodeStat(const Blob::LightStatData_t& stat){
	cJSON* light = NULL;
	cJSON* value = NULL;

	if((light=cJSON_CreateObject()) == NULL){
		return NULL;
	}

	cJSON_AddNumberToObject(light, p_flags, stat.flags);
	cJSON_AddNumberToObject(light, p_outValue, stat.outValue);
	return light;
}


//------------------------------------------------------------------------------------
bool LightManager::decodeSetRequest(Blob::SetRequest_t<Blob::LightCfgData_t>&req, char* json_data){
	cJSON *alsData = NULL;
	cJSON *outData = NULL;
	cJSON *array = NULL;
	cJSON *value = NULL;
	cJSON *luxLevel = NULL;
	cJSON *item = NULL;
	cJSON *obj = NULL;
	cJSON *light = NULL;
	cJSON *root = NULL;
	req.keys = Blob::LightKeyNone;
	req._error.code = Blob::ErrOK;
	strcpy(req._error.descr, Blob::errList[req._error.code]);

	if((root = cJSON_Parse(json_data)) == NULL){
		req._error.code = Blob::ErrJsonMalformed;
		req.idTrans = Blob::UnusedIdTrans;
		goto __decode_null_exitLightCfg;
	}

	// key: idTrans
	if((obj = cJSON_GetObjectItem(root, p_idTrans)) == NULL){
		req._error.code = Blob::ErrIdTransInvalid;
		req.idTrans = Blob::UnusedIdTrans;
		goto __decode_null_exitLightCfg;
	}
	req.idTrans = obj->valueint;

	//key: light
	if((light = cJSON_GetObjectItem(root, p_light)) == NULL){
		req._error.code = Blob::ErrLightMissing;
		goto __decode_null_exitLightCfg;
	}

	if((obj = cJSON_GetObjectItem(light, p_updFlags)) != NULL){
		req.data.updFlagMask = (Blob::LightUpdFlags)obj->valueint;
		req.keys |= Blob::LightKeyCfgUpd;
	}
	if((obj = cJSON_GetObjectItem(light, p_evtFlags)) != NULL){
		req.data.evtFlagMask = (Blob::LightEvtFlags)obj->valueint;
		req.keys |= Blob::LightKeyCfgEvt;
	}
	//key: alsData
	if((alsData = cJSON_GetObjectItem(light, p_alsData)) != NULL){
		cJSON *lux = NULL;
		if((lux = cJSON_GetObjectItem(alsData, p_lux)) != NULL){
			if((obj = cJSON_GetObjectItem(lux, p_min)) != NULL){
				req.data.alsData.lux.min = obj->valueint;
			}
			if((obj = cJSON_GetObjectItem(lux, p_max)) != NULL){
				req.data.alsData.lux.max = obj->valueint;
			}
			if((obj = cJSON_GetObjectItem(lux, p_thres)) != NULL){
				req.data.alsData.lux.thres = obj->valueint;
			}
			req.keys |= Blob::LightKeyCfgAls;
		}
	}

	if((outData = cJSON_GetObjectItem(light, p_outData)) != NULL){
		if((obj = cJSON_GetObjectItem(outData, p_mode)) != NULL){
			req.data.outData.mode = (Blob::LightOutModeFlags)obj->valueint;
			req.keys |= Blob::LightKeyCfgOutm;
		}
		if((obj = cJSON_GetObjectItem(outData, p_numActions)) != NULL){
			req.data.outData.numActions = obj->valueint;
		}

		if((value = cJSON_GetObjectItem(outData, p_curve)) != NULL){
			if((obj = cJSON_GetObjectItem(value, p_samples)) != NULL){
				req.data.outData.curve.samples = obj->valueint;
			}
			if((obj = cJSON_GetObjectItem(value, p_data)) != NULL){
				for(int i=0;i<cJSON_GetArraySize(obj);i++){
					item = cJSON_GetArrayItem(obj, i);
					req.data.outData.curve.data[i] = item->valueint;
				}
			}
			req.keys |= Blob::LightKeyCfgCurve;
		}

		if((array = cJSON_GetObjectItem(outData, p_actions)) != NULL){
			for(int i=0;i<cJSON_GetArraySize(array);i++){
				value = cJSON_GetArrayItem(array, i);
				if((obj = cJSON_GetObjectItem(value, p_id)) != NULL){
					req.data.outData.actions[i].id = obj->valueint;
				}
				if((obj = cJSON_GetObjectItem(value, p_flags)) != NULL){
					req.data.outData.actions[i].flags = (Blob::LightActionFlags)obj->valueint;
				}
				if((obj = cJSON_GetObjectItem(value, p_date)) != NULL){
					req.data.outData.actions[i].date = obj->valueint;
				}
				if((obj = cJSON_GetObjectItem(value, p_time)) != NULL){
					req.data.outData.actions[i].time = obj->valueint;
				}
				if((obj = cJSON_GetObjectItem(value, p_astCorr)) != NULL){
					req.data.outData.actions[i].astCorr = obj->valueint;
				}
				if((obj = cJSON_GetObjectItem(value, p_outValue)) != NULL){
					req.data.outData.actions[i].outValue = obj->valueint;
				}

				if((luxLevel = cJSON_GetObjectItem(value, p_luxLevel)) != NULL){
					if((obj = cJSON_GetObjectItem(luxLevel, p_min)) != NULL){
						req.data.outData.actions[i].luxLevel.min = obj->valueint;
					}
					if((obj = cJSON_GetObjectItem(luxLevel, p_max)) != NULL){
						req.data.outData.actions[i].luxLevel.max = obj->valueint;
					}
					if((obj = cJSON_GetObjectItem(luxLevel, p_thres)) != NULL){
						req.data.outData.actions[i].luxLevel.thres = obj->valueint;
					}
				}
			}
			req.keys |= Blob::LightKeyCfgActs;
		}
	}

__decode_null_exitLightCfg:
	strcpy(req._error.descr, Blob::errList[req._error.code]);
	if(root){
		cJSON_Delete(root);
	}
	if(req._error.code == Blob::ErrOK){
		return true;
	}
	return false;
}


//------------------------------------------------------------------------------------
cJSON* LightManager::encodeCfgResponse(const Blob::Response_t<Blob::LightCfgData_t> &resp){
	// keys: root, idtrans, header, error, astcal
	cJSON *header = NULL;
	cJSON *error = NULL;
	cJSON *light = NULL;
	cJSON *value = NULL;
	cJSON *root = cJSON_CreateObject();

	if(!root){
		return NULL;
	}

	// key: idTrans sólo se envía si está en uso
	if(resp.idTrans != Blob::UnusedIdTrans){
		cJSON_AddNumberToObject(root, p_idTrans, resp.idTrans);
	}

	// key: header
	if((header=cJSON_CreateObject()) == NULL){
		goto _parseLightCfg_Exit;
	}
	cJSON_AddNumberToObject(header, p_timestamp, resp.header.timestamp);
	cJSON_AddItemToObject(root, p_header, header);

	// key: error sólo se envía si el error es distinto de Blob::ErrOK
	if(resp.error.code != Blob::ErrOK){
		if((error=cJSON_CreateObject()) == NULL){
			goto _parseLightCfg_Exit;
		}
		cJSON_AddNumberToObject(error, p_code, resp.error.code);
		if((value=cJSON_CreateString(resp.error.descr)) == NULL){
			goto _parseLightCfg_Exit;
		}
		cJSON_AddItemToObject(error, p_descr, value);
		// añade objeto
		cJSON_AddItemToObject(root, p_error, error);
	}

	// key: astcal sólo se envía si el error es Blob::ErrOK
	if(resp.error.code == Blob::ErrOK){
		if((light = encodeCfg(resp.data)) == NULL){
			goto _parseLightCfg_Exit;
		}
		// añade objeto
		cJSON_AddItemToObject(root, p_light, light);
	}
	return root;

_parseLightCfg_Exit:
	cJSON_Delete(root);
	return NULL;
}


//------------------------------------------------------------------------------------
cJSON* LightManager::encodeStatResponse(const Blob::Response_t<Blob::LightStatData_t> &resp){
	// keys: root, idtrans, header, error, energy
	cJSON *header = NULL;
	cJSON *error = NULL;
	cJSON *light = NULL;
	cJSON *value = NULL;
	cJSON *root = cJSON_CreateObject();

	if(!root){
		return NULL;
	}

	// key: idTrans sólo se envía si está en uso
	if(resp.idTrans != Blob::UnusedIdTrans){
		cJSON_AddNumberToObject(root, p_idTrans, resp.idTrans);
	}

	// key: header
	if((header=cJSON_CreateObject()) == NULL){
		goto _parseLightStat_Exit;
	}
	cJSON_AddNumberToObject(header, p_timestamp, resp.header.timestamp);
	cJSON_AddItemToObject(root, p_header, header);

	// key: error sólo se envía si el error es distinto de Blob::ErrOK
	if(resp.error.code != Blob::ErrOK){
		if((error=cJSON_CreateObject()) == NULL){
			goto _parseLightStat_Exit;
		}
		cJSON_AddNumberToObject(error, p_code, resp.error.code);
		if((value=cJSON_CreateString(resp.error.descr)) == NULL){
			goto _parseLightStat_Exit;
		}
		cJSON_AddItemToObject(error, p_descr, value);
		// añade objeto
		cJSON_AddItemToObject(root, p_error, error);
	}

	// key: astcal sólo se envía si el error es Blob::ErrOK
	if(resp.error.code == Blob::ErrOK){
		if((light = encodeStat(resp.data)) == NULL){
			goto _parseLightStat_Exit;
		}
		// añade objeto
		cJSON_AddItemToObject(root, p_light, light);
	}
	return root;

	_parseLightStat_Exit:
	cJSON_Delete(root);
	return NULL;
}


//------------------------------------------------------------------------------------
cJSON* LightManager::encodeBoot(const Blob::LightBootData_t& boot){
	cJSON* light = NULL;
	cJSON* item = NULL;
	if((light=cJSON_CreateObject()) == NULL){
		return NULL;
	}

	if((item = encodeCfg(boot.cfg)) == NULL){
		goto __encodeBoot_Err;
	}
	cJSON_AddItemToObject(light, p_cfg, item);

	if((item = encodeStat(boot.stat)) == NULL){
		goto __encodeBoot_Err;
	}
	cJSON_AddItemToObject(light, p_stat, item);
	return light;

__encodeBoot_Err:
	cJSON_Delete(light);
	return NULL;
}


//------------------------------------------------------------------------------------
static bool LightManager::_decodeSetReqValue(Blob::SetRequest_t<Blob::LightStatData_t>&req, char* json_data){
	cJSON *alsData = NULL;
	cJSON *outData = NULL;
	cJSON *array = NULL;
	cJSON *value = NULL;
	cJSON *luxLevel = NULL;
	cJSON *item = NULL;
	cJSON *obj = NULL;
	cJSON *light = NULL;
	cJSON *root = NULL;
	req.keys = Blob::LightKeyNone;
	req._error.code = Blob::ErrOK;
	strcpy(req._error.descr, Blob::errList[req._error.code]);

	if((root = cJSON_Parse(json_data)) == NULL){
		req._error.code = Blob::ErrJsonMalformed;
		req.idTrans = Blob::UnusedIdTrans;
		goto _decodeSetReqValue_Exit;
	}

	// key: idTrans
	if((obj = cJSON_GetObjectItem(root, p_idTrans)) == NULL){
		req._error.code = Blob::ErrIdTransInvalid;
		req.idTrans = Blob::UnusedIdTrans;
		goto _decodeSetReqValue_Exit;
	}
	req.idTrans = obj->valueint;

	//key: light
	if((light = cJSON_GetObjectItem(root, p_light)) == NULL){
		req._error.code = Blob::ErrLightMissing;
		goto _decodeSetReqValue_Exit;
	}

	if((obj = cJSON_GetObjectItem(light, p_flags)) != NULL){
		req.data.flags = obj->valueint;
	}
	if((obj = cJSON_GetObjectItem(light, p_outValue)) != NULL){
		req.data.outValue = obj->valueint;
	}

_decodeSetReqValue_Exit:
	strcpy(req._error.descr, Blob::errList[req._error.code]);
	if(root){
		cJSON_Delete(root);
	}
	if(req._error.code == Blob::ErrOK){
		return true;
	}
	return false;
}


//------------------------------------------------------------------------------------
static bool LightManager::_decodeSetValue(Blob::LightStatData_t &req, char* json_data){
	cJSON *root = NULL;
	cJSON *astData = NULL;
	cJSON *obj = NULL;
	if((root = cJSON_Parse(json_data)) == NULL){
		return false;
	}

	// key: flags
	if((obj = cJSON_GetObjectItem(root, p_flags)) == NULL){
		cJSON_Delete(root);
		return false;
	}
	req.flags = obj->valueint;

	// key: outValue
	if((obj = cJSON_GetObjectItem(root, p_outValue)) == NULL){
		cJSON_Delete(root);
		return false;
	}
	req.outValue = obj->valueint;

	cJSON_Delete(root);
	return true;
}


//------------------------------------------------------------------------------------
static bool LightManager::_decodeSetLux(Blob::LightLuxLevel &req, char* json_data){
	cJSON *root = NULL;
	cJSON *obj = NULL;
	if((root = cJSON_Parse(json_data)) == NULL){
		return false;
	}

	// key: outValue
	if((obj = cJSON_GetObjectItem(root, p_outValue)) == NULL){
		cJSON_Delete(root);
		return false;
	}
	req = obj->valueint;
	cJSON_Delete(root);
	return true;
}


//------------------------------------------------------------------------------------
static bool LightManager::_decodeSetTime(Blob::AstCalStatData_t &req, char* json_data){
	cJSON *root = NULL;
	cJSON *astData = NULL;
	cJSON *obj = NULL;
	if((root = cJSON_Parse(json_data)) == NULL){
		return false;
	}

	// key: flags
	if((obj = cJSON_GetObjectItem(root, p_flags)) == NULL){
		cJSON_Delete(root);
		return false;
	}
	req.flags = obj->valueint;

	// key: period
	if((obj = cJSON_GetObjectItem(root, p_period)) == NULL){
		cJSON_Delete(root);
		return false;
	}
	req.period = obj->valueint;

	// key: now
	if((obj = cJSON_GetObjectItem(root, p_now)) == NULL){
		cJSON_Delete(root);
		return false;
	}
	req.now = (time_t)obj->valuedouble;

	// key: astData
	if((astData = cJSON_GetObjectItem(root, p_now)) == NULL){
		cJSON_Delete(root);
		return false;
	}

	if((obj = cJSON_GetObjectItem(root, p_latitude)) == NULL){
		cJSON_Delete(root);
		return false;
	}
	req.astData.latitude = obj->valueint;
	if((obj = cJSON_GetObjectItem(root, p_longitude)) == NULL){
		cJSON_Delete(root);
		return false;
	}
	req.astData.longitude = obj->valueint;
	if((obj = cJSON_GetObjectItem(root, p_wdowDawnStart)) == NULL){
		cJSON_Delete(root);
		return false;
	}
	req.astData.wdowDawnStart = obj->valueint;
	if((obj = cJSON_GetObjectItem(root, p_wdowDawnStop)) == NULL){
		cJSON_Delete(root);
		return false;
	}
	req.astData.wdowDawnStop = obj->valueint;
	if((obj = cJSON_GetObjectItem(root, p_wdowDuskStart)) == NULL){
		cJSON_Delete(root);
		return false;
	}
	req.astData.wdowDuskStart = obj->valueint;
	if((obj = cJSON_GetObjectItem(root, p_wdowDuskStop)) == NULL){
		cJSON_Delete(root);
		return false;
	}
	req.astData.wdowDuskStop = obj->valueint;
	if((obj = cJSON_GetObjectItem(root, p_reductionStart)) == NULL){
		cJSON_Delete(root);
		return false;
	}
	req.astData.reductionStart = obj->valueint;
	if((obj = cJSON_GetObjectItem(root, p_reductionStop)) == NULL){
		cJSON_Delete(root);
		return false;
	}
	req.astData.reductionStop = obj->valueint;
	cJSON_Delete(root);
	return true;
}
