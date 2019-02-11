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
osEvent LightManager:: getOsEvent(){
	return _queue.get();
}



//------------------------------------------------------------------------------------
void LightManager::publicationCb(const char* topic, int32_t result){

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

		if(_json_supported){
			cJSON* jboot = JsonParser::getJsonFromObj(_lightdata.stat);
			if(jboot){
				char* jmsg = cJSON_Print(jboot);
				cJSON_Delete(jboot);
				MQ::MQClient::publish(pub_topic, jmsg, strlen(jmsg)+1, &_publicationCb);
				Heap::memFree(jmsg);
				Heap::memFree(pub_topic);
				return;
			}
		}

		MQ::MQClient::publish(pub_topic, &_lightdata.stat, sizeof(Blob::LightStatData_t), &_publicationCb);
		Heap::memFree(pub_topic);
	}
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

