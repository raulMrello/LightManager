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
#define _EXPR_	(!IS_ISR())

 
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
	_lightdata.uid = UID_LIGHT_MANAGER;
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
	_lightdata.cfg.verbosity = ESP_LOG_DEBUG;

	saveConfig();
}


//------------------------------------------------------------------------------------
void LightManager::restoreConfig(){
	uint32_t crc = 0;
	_lightdata.uid = UID_LIGHT_MANAGER;
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
	if(!restoreParameter("LightVerbosity", &_lightdata.cfg.verbosity, sizeof(esp_log_level_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo verbosity!");
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
    		esp_log_level_set(_MODULE_, _lightdata.cfg.verbosity);
    		_sched->setVerbosity(_lightdata.cfg.verbosity);
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
	if(!saveParameter("LightVerbosity", &_lightdata.cfg.verbosity, sizeof(esp_log_level_t), NVSInterface::TypeUint32)){
		DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS grabando verbosity!");
	}
	else{
		esp_log_level_set(_MODULE_, _lightdata.cfg.verbosity);
		_sched->setVerbosity(_lightdata.cfg.verbosity);
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
	Blob::NotificationData_t<Blob::LightStatData_t> *notif = new Blob::NotificationData_t<Blob::LightStatData_t>(_lightdata.stat);
	MBED_ASSERT(notif);
	if(_json_supported){
		cJSON* jboot = JsonParser::getJsonFromNotification(*notif);
		MBED_ASSERT(jboot);
		MQ::MQClient::publish(pub_topic, &jboot, sizeof(cJSON**), &_publicationCb);
		cJSON_Delete(jboot);
	}
	else{
		MQ::MQClient::publish(pub_topic, notif, sizeof(Blob::NotificationData_t<Blob::LightStatData_t>), &_publicationCb);
	}
	delete(notif);
	Heap::memFree(pub_topic);
}


// ----------------------------------------------------------------------------------
void LightManager::_updateConfig(const light_manager& data, Blob::ErrorData_t& err){
	if(data.cfg._keys & Blob::LightKeyCfgUpd){
		_lightdata.cfg.updFlagMask = data.cfg.updFlagMask;
	}
	if(data.cfg._keys & Blob::LightKeyCfgEvt){
		_lightdata.cfg.evtFlagMask = data.cfg.evtFlagMask;
	}
	if(data.cfg._keys & Blob::LightKeyCfgAls){
		_lightdata.cfg.alsData = data.cfg.alsData;
	}
	if(data.cfg._keys & Blob::LightKeyCfgOutm){
		_lightdata.cfg.outData.mode = data.cfg.outData.mode;
	}
	if(data.cfg._keys & Blob::LightKeyCfgCurve){
		_lightdata.cfg.outData.curve = data.cfg.outData.curve;
	}
	if(data.cfg._keys & Blob::LightKeyCfgActs){
		_lightdata.cfg.outData.numActions = data.cfg.outData.numActions;
		for(int i=0;i<data.cfg.outData.numActions;i++)
			_lightdata.cfg.outData.actions[data.cfg.outData.actions[i].id] = data.cfg.outData.actions[i];
	}
	if(data.cfg._keys & Blob::LightKeyCfgVerbosity){
		_lightdata.cfg.verbosity = data.cfg.verbosity;
	}
	strcpy(err.descr, Blob::errList[err.code]);
}



