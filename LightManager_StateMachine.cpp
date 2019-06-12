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

/** Macro para imprimir trazas de depuraci�n, siempre que se haya configurado un objeto
 *	Logger v�lido (ej: _debug)
 */
static const char* _MODULE_ = "[LightM]........";
#define _EXPR_	(!IS_ISR())

 
//------------------------------------------------------------------------------------
State::StateResult LightManager::Init_EventHandler(State::StateEvent* se){
	State::Msg* st_msg = (State::Msg*)se->oe->value.p;
    switch((int)se->evt){
        case State::EV_ENTRY:{
        	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Iniciando recuperaci�n de datos...");
        	// recupera los datos de memoria NV
        	restoreConfig();

        	// realiza la suscripci�n local ej: "cmd/$module/#"
        	char* sub_topic_local = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
        	MBED_ASSERT(sub_topic_local);
        	sprintf(sub_topic_local, "set/+/%s", _sub_topic_base);
        	if(MQ::MQClient::subscribe(sub_topic_local, new MQ::SubscribeCallback(this, &LightManager::subscriptionCb)) == MQ::SUCCESS){
        		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Sucripci�n LOCAL hecha a %s", sub_topic_local);
        	}
        	else{
        		DEBUG_TRACE_E(_EXPR_, _MODULE_, "ERR_SUBSC en la suscripci�n LOCAL a %s", sub_topic_local);
        	}
        	sprintf(sub_topic_local, "get/+/%s", _sub_topic_base);
        	if(MQ::MQClient::subscribe(sub_topic_local, new MQ::SubscribeCallback(this, &LightManager::subscriptionCb)) == MQ::SUCCESS){
        		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Sucripci�n LOCAL hecha a %s", sub_topic_local);
        	}
        	else{
        		DEBUG_TRACE_E(_EXPR_, _MODULE_, "ERR_SUBSC en la suscripci�n LOCAL a %s", sub_topic_local);
        	}
        	Heap::memFree(sub_topic_local);

        	// marca como componente iniciado
        	_ready = true;
            return State::HANDLED;
        }

        case State::EV_TIMED:{
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicaci�n en cmd/$BASE/cfg/set
        case RecvCfgSet:{
        	Blob::SetRequest_t<light_manager>* req = (Blob::SetRequest_t<light_manager>*)st_msg->msg;
			// si no hay errores, actualiza la configuraci�n
			if(req->_error.code == Blob::ErrOK){
				_updateConfig(req->data, req->_error);
			}
        	// si hay errores en el mensaje o en la actualizaci�n, devuelve resultado sin hacer nada
        	if(req->_error.code != Blob::ErrOK){
        		char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
				MBED_ASSERT(pub_topic);
				sprintf(pub_topic, "stat/cfg/%s", _pub_topic_base);

				Blob::Response_t<light_manager>* resp = new Blob::Response_t<light_manager>(req->idTrans, req->_error, _lightdata);

				if(_json_supported){
					cJSON* jresp = JsonParser::getJsonFromResponse(*resp, ObjSelectCfg);
					MBED_ASSERT(jresp);
					MQ::MQClient::publish(pub_topic, &jresp, sizeof(cJSON**), &_publicationCb);
					cJSON_Delete(jresp);
				}
				else{
					MQ::MQClient::publish(pub_topic, resp, sizeof(Blob::Response_t<light_manager>), &_publicationCb);
				}
				delete(resp);
				Heap::memFree(pub_topic);
				return State::HANDLED;
        	}

        	// almacena en el sistema de ficheros
        	saveConfig();

        	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Config actualizada");

        	// si est� habilitada la notificaci�n de actualizaci�n, lo notifica
        	if((_lightdata.cfg.updFlagMask & Blob::EnableLightCfgUpdNotif) != 0){
				char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
				MBED_ASSERT(pub_topic);
				sprintf(pub_topic, "stat/cfg/%s", _pub_topic_base);
				Blob::Response_t<light_manager>* resp = new Blob::Response_t<light_manager>(req->idTrans, req->_error, _lightdata);

				if(_json_supported){
					cJSON* jresp = JsonParser::getJsonFromResponse(*resp, ObjSelectCfg);
					MBED_ASSERT(jresp);
					MQ::MQClient::publish(pub_topic, &jresp, sizeof(cJSON**), &_publicationCb);
					cJSON_Delete(jresp);
				}
				else{
					MQ::MQClient::publish(pub_topic, resp, sizeof(Blob::Response_t<light_manager>), &_publicationCb);
				}
				delete(resp);
				Heap::memFree(pub_topic);
        	}

            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicaci�n en cmd/$BASE/value/set
        case RecvStatSet:{
        	Blob::SetRequest_t<light_manager>* req = (Blob::SetRequest_t<light_manager>*)st_msg->msg;
        	// si el mensaje recibido no tiene errores de preprocesado contin�a
        	if(req->_error.code == Blob::ErrOK){
				// actualiza el estado de la luminaria
				uint8_t value =  req->data.stat.outValue;
				// si la salida est� fuera de rango, no hace nada y devuelve un error
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
					// cambio de estado de regulaci�n
					else{
						_lightdata.stat.flags = Blob::LightOutLevelChangeEvt;
					}
					_lightdata.stat.outValue = value;
					if(_driver010){
						_driver010->setLevel(_applyCurve(_lightdata.stat.outValue));
					}

					DEBUG_TRACE_I(_EXPR_, _MODULE_, "LIGHT_VALUE_SET, actualizado = %d", _lightdata.stat.outValue);
				}
        	}

        	// notifica el cambio de estado
        	char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
			MBED_ASSERT(pub_topic);
			sprintf(pub_topic, "stat/value/%s", _pub_topic_base);
			Blob::Response_t<light_manager>* resp = new Blob::Response_t<light_manager>(req->idTrans, req->_error, _lightdata);
			MBED_ASSERT(resp);
			if(_json_supported){
				cJSON* jresp = JsonParser::getJsonFromResponse(*resp, ObjSelectState);
				MBED_ASSERT(jresp);
				MQ::MQClient::publish(pub_topic, &jresp, sizeof(cJSON**), &_publicationCb);
				cJSON_Delete(jresp);
			}
			else{
				MQ::MQClient::publish(pub_topic, resp, sizeof(Blob::Response_t<light_manager>), &_publicationCb);
			}
			delete(resp);
			Heap::memFree(pub_topic);
        	return State::HANDLED;
        }

        // Procesa datos recibidos de la publicaci�n en cmd/$BASE/cfg/get
        case RecvCfgGet:{
        	Blob::GetRequest_t* req = (Blob::GetRequest_t*)st_msg->msg;

			DEBUG_TRACE_I(_EXPR_, _MODULE_, "Respondiendo datos de configuraci�n solicitados");
			char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
			MBED_ASSERT(pub_topic);
			sprintf(pub_topic, "stat/cfg/%s", _pub_topic_base);

			// responde con los datos solicitados y con los errores (si hubiera) de la decodificaci�n de la solicitud
			Blob::Response_t<light_manager>* resp = new Blob::Response_t<light_manager>(req->idTrans, req->_error, _lightdata);
			MBED_ASSERT(resp);
			if(_json_supported){
				cJSON* jresp = JsonParser::getJsonFromResponse(*resp, ObjSelectCfg);
				MBED_ASSERT(jresp);
				MQ::MQClient::publish(pub_topic, &jresp, sizeof(cJSON**), &_publicationCb);
				cJSON_Delete(jresp);
			}
			else{
				MQ::MQClient::publish(pub_topic, resp, sizeof(Blob::Response_t<light_manager>), &_publicationCb);
			}
			delete(resp);
			Heap::memFree(pub_topic);
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicaci�n en cmd/$BASE/value/get
        case RecvStatGet:{
        	Blob::GetRequest_t* req = (Blob::GetRequest_t*)st_msg->msg;
        	// prepara el topic al que responder
        	char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
			MBED_ASSERT(pub_topic);
			sprintf(pub_topic, "stat/value/%s", _pub_topic_base);

			DEBUG_TRACE_I(_EXPR_, _MODULE_, "Respondiendo datos de estado solicitados");

			// responde con los datos solicitados y con los errores (si hubiera) de la decodificaci�n de la solicitud
			Blob::Response_t<light_manager>* resp = new Blob::Response_t<light_manager>(req->idTrans, req->_error, _lightdata);
			MBED_ASSERT(resp);

			// borra los flags de evento ya que es una respuesta sin m�s
			resp->data.stat.flags = Blob::LightNoEvents;

			if(_json_supported){
				cJSON* jresp = JsonParser::getJsonFromResponse(*resp, ObjSelectState);
				MBED_ASSERT(jresp);
				MQ::MQClient::publish(pub_topic, &jresp, sizeof(cJSON**), &_publicationCb);
				cJSON_Delete(jresp);
			}
			else {
				MQ::MQClient::publish(pub_topic, resp, sizeof(Blob::Response_t<light_manager>), &_publicationCb);
			}
			delete(resp);
			Heap::memFree(pub_topic);
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicaci�n en get/boot
        case RecvBootGet:{
        	char* pub_topic = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
			MBED_ASSERT(pub_topic);
			sprintf(pub_topic, "stat/boot/%s", _pub_topic_base);
			Blob::NotificationData_t<light_manager> *notif = new Blob::NotificationData_t<light_manager>(_lightdata);
			MBED_ASSERT(notif);
			if(_json_supported){
				cJSON* jboot = JsonParser::getJsonFromNotification(*notif, ObjSelectAll);
				MBED_ASSERT(jboot);
				MQ::MQClient::publish(pub_topic, &jboot, sizeof(cJSON**), &_publicationCb);
				cJSON_Delete(jboot);
			}
			else {
				MQ::MQClient::publish(pub_topic, notif, sizeof(Blob::NotificationData_t<light_manager>), &_publicationCb);
			}
			delete(notif);
			Heap::memFree(pub_topic);
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicaci�n en set/time
        case RecvTimeSet:{
        	Blob::LightTimeData_t ast = *(Blob::LightTimeData_t*)st_msg->msg;
			int8_t new_out_value;
			// ejecuta el scheduler y en caso de que haya un nuevo estado de la carga, lo notifica
			if((new_out_value = _sched->updateTimestamp(ast)) != -1){
				_updateAndNotify(new_out_value);
			}
            return State::HANDLED;
        }

        // Procesa datos recibidos de la publicaci�n en set/lux
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


