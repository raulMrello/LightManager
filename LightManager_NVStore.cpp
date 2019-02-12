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



