/*
 * LightManager_Json.cpp
 *
 * Implementación de los codecs JSON-OBJ
 *
 *  Created on: Feb 2019
 *      Author: raulMrello
 */

#include "JsonParserBlob.h"

//------------------------------------------------------------------------------------
//-- PRIVATE TYPEDEFS ----------------------------------------------------------------
//------------------------------------------------------------------------------------

/** Macro para imprimir trazas de depuración, siempre que se haya configurado un objeto
 *	Logger válido (ej: _debug)
 */
static const char* _MODULE_ = "[LightM]........";
#define _EXPR_	(true)

 


namespace JSON {

//------------------------------------------------------------------------------------
cJSON* getJsonFromLightCfg(const Blob::LightCfgData_t& cfg){
	cJSON *alsData = NULL;
	cJSON *outData = NULL;
	cJSON *curve = NULL;
	cJSON *array = NULL;
	cJSON *value = NULL;
	cJSON *luxLevel = NULL;
	cJSON *light = NULL;

	if((light=cJSON_CreateObject()) == NULL){
		DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error creando json");
		return NULL;
	}

	// key: light.updFlags
	cJSON_AddNumberToObject(light, JsonParser::p_updFlags, cfg.updFlagMask);

	// key: light.evtFlags
	cJSON_AddNumberToObject(light, JsonParser::p_evtFlags, cfg.evtFlagMask);

	// key: fwupd.verbosity
	cJSON_AddNumberToObject(light, JsonParser::p_verbosity, cfg.verbosity);

	// key: alsData
	if((alsData=cJSON_CreateObject()) == NULL){
		cJSON_Delete(light);
		DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error creando alsData");
		return NULL;
	}

	// key: alsData.lux
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(alsData);
		cJSON_Delete(light);
		DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error creando alsData.lux");
		return NULL;
	}
	cJSON_AddNumberToObject(value, JsonParser::p_min, cfg.alsData.lux.min);
	cJSON_AddNumberToObject(value, JsonParser::p_max, cfg.alsData.lux.max);
	cJSON_AddNumberToObject(value, JsonParser::p_thres, cfg.alsData.lux.thres);
	cJSON_AddItemToObject(alsData, JsonParser::p_lux, value);
	cJSON_AddItemToObject(light, JsonParser::p_alsData, alsData);

	// key: outData
	if((outData=cJSON_CreateObject()) == NULL){
		cJSON_Delete(light);
		DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error creando outData");
		return NULL;
	}
	cJSON_AddNumberToObject(outData, JsonParser::p_mode, cfg.outData.mode);
	cJSON_AddNumberToObject(outData, JsonParser::p_numActions, cfg.outData.numActions);

	// key: outData.curve
	if((curve=cJSON_CreateObject()) == NULL){
		cJSON_Delete(outData);
		cJSON_Delete(light);
		DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error creando outData.curve");
		return NULL;
	}
	cJSON_AddNumberToObject(curve, JsonParser::p_samples, cfg.outData.curve.samples);

	// key: outData.curve.data
	if((array=cJSON_CreateArray()) == NULL){
		cJSON_Delete(curve);
		cJSON_Delete(outData);
		cJSON_Delete(light);
		DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error creando outData.curve.data[]");
		return NULL;
	}
	for(int i=0; i < cfg.outData.curve.samples; i++){
		if((value = cJSON_CreateNumber(cfg.outData.curve.data[i])) == NULL){
			cJSON_Delete(array);
			cJSON_Delete(curve);
			cJSON_Delete(outData);
			cJSON_Delete(light);
			DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error creando outData.curve.data[%d]",i);
			return NULL;
		}
		cJSON_AddItemToArray(array, value);
	}
	cJSON_AddItemToObject(curve, JsonParser::p_data, array);
	cJSON_AddItemToObject(outData, JsonParser::p_curve, curve);

	// key: outData.actions
	if((array=cJSON_CreateArray()) == NULL){
		cJSON_Delete(outData);
		cJSON_Delete(light);
		DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error creando outData.actions[]");
		return NULL;
	}

	for(int i=0; i < cfg.outData.numActions; i++){
		if((value = cJSON_CreateObject()) == NULL){
			cJSON_Delete(array);
			cJSON_Delete(outData);
			cJSON_Delete(light);
			DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error creando outData.actions[%d]",i);
			return NULL;
		}
		cJSON_AddNumberToObject(value, JsonParser::p_id, cfg.outData.actions[i].id);
		cJSON_AddNumberToObject(value, JsonParser::p_flags, cfg.outData.actions[i].flags);
		cJSON_AddNumberToObject(value, JsonParser::p_date, cfg.outData.actions[i].date);
		cJSON_AddNumberToObject(value, JsonParser::p_time, cfg.outData.actions[i].time);
		cJSON_AddNumberToObject(value, JsonParser::p_astCorr, cfg.outData.actions[i].astCorr);
		cJSON_AddNumberToObject(value, JsonParser::p_outValue, cfg.outData.actions[i].outValue);
		if((luxLevel=cJSON_CreateObject()) == NULL){
			cJSON_Delete(value);
			cJSON_Delete(array);
			cJSON_Delete(outData);
			cJSON_Delete(light);
			DEBUG_TRACE_E(_EXPR_, _MODULE_, "Error creando outData.curve.data[%d].luxLevel",i);
			return NULL;
		}
		cJSON_AddNumberToObject(luxLevel, JsonParser::p_min, cfg.outData.actions[i].luxLevel.min);
		cJSON_AddNumberToObject(luxLevel, JsonParser::p_max, cfg.outData.actions[i].luxLevel.max);
		cJSON_AddNumberToObject(luxLevel, JsonParser::p_thres, cfg.outData.actions[i].luxLevel.thres);
		cJSON_AddItemToObject(value, JsonParser::p_luxLevel, luxLevel);
		cJSON_AddItemToArray(array, value);
	}
	cJSON_AddItemToObject(outData, JsonParser::p_actions, array);
	cJSON_AddItemToObject(light, JsonParser::p_outData, outData);
	return light;
}


//------------------------------------------------------------------------------------
cJSON* getJsonFromLightStat(const Blob::LightStatData_t& stat){
	cJSON* json = NULL;

	if((json=cJSON_CreateObject()) == NULL){
		return NULL;
	}

	cJSON_AddNumberToObject(json, JsonParser::p_flags, stat.flags);
	cJSON_AddNumberToObject(json, JsonParser::p_outValue, stat.outValue);
	return json;
}


//------------------------------------------------------------------------------------
cJSON* getJsonFromLightBoot(const Blob::LightBootData_t& boot){
	cJSON* json = NULL;
	cJSON* item = NULL;
	if((json=cJSON_CreateObject()) == NULL){
		return NULL;
	}

	if((item = getJsonFromLightCfg(boot.cfg)) == NULL){
		cJSON_Delete(json);
		return NULL;
	}
	cJSON_AddItemToObject(json, JsonParser::p_cfg, item);

	if((item = getJsonFromLightStat(boot.stat)) == NULL){
		cJSON_Delete(json);
		return NULL;
	}
	cJSON_AddItemToObject(json, JsonParser::p_stat, item);
	return json;
}


//------------------------------------------------------------------------------------
cJSON* getJsonFromLightLux(const Blob::LightLuxLevel& lux){
	cJSON* json = NULL;
	if((json=cJSON_CreateObject()) == NULL){
		return NULL;
	}
	cJSON_AddNumberToObject(json, JsonParser::p_luxLevel, lux);
	return json;
}


//------------------------------------------------------------------------------------
cJSON* getJsonFromLightTime(const Blob::LightTimeData_t& t){
	return JSON::getJsonFromAstCalStat(t);
}


//------------------------------------------------------------------------------------
uint32_t getLightCfgFromJson(Blob::LightCfgData_t &cfg, cJSON* json){
	cJSON *alsData = NULL;
	cJSON *outData = NULL;
	cJSON *array = NULL;
	cJSON *value = NULL;
	cJSON *luxLevel = NULL;
	cJSON *item = NULL;
	cJSON *obj = NULL;
	uint32_t keys = Blob::LightKeyNone;

	if((obj = cJSON_GetObjectItem(json, JsonParser::p_updFlags)) != NULL){
		cfg.updFlagMask = (Blob::LightUpdFlags)obj->valueint;
		keys |= Blob::LightKeyCfgUpd;
	}
	if((obj = cJSON_GetObjectItem(json, JsonParser::p_evtFlags)) != NULL){
		cfg.evtFlagMask = (Blob::LightEvtFlags)obj->valueint;
		keys |= Blob::LightKeyCfgEvt;
	}
	if((obj = cJSON_GetObjectItem(json,JsonParser::p_verbosity)) != NULL){
		cfg.verbosity = obj->valueint;
		keys |= Blob::LightKeyCfgVerbosity;
	}

	//key: alsData
	if((alsData = cJSON_GetObjectItem(json, JsonParser::p_alsData)) != NULL){
		cJSON *lux = NULL;
		if((lux = cJSON_GetObjectItem(alsData, JsonParser::p_lux)) != NULL){
			if((obj = cJSON_GetObjectItem(lux, JsonParser::p_min)) != NULL){
				cfg.alsData.lux.min = obj->valueint;
			}
			if((obj = cJSON_GetObjectItem(lux, JsonParser::p_max)) != NULL){
				cfg.alsData.lux.max = obj->valueint;
			}
			if((obj = cJSON_GetObjectItem(lux, JsonParser::p_thres)) != NULL){
				cfg.alsData.lux.thres = obj->valueint;
			}
			keys |= Blob::LightKeyCfgAls;
		}
	}

	if((outData = cJSON_GetObjectItem(json, JsonParser::p_outData)) != NULL){
		if((obj = cJSON_GetObjectItem(outData, JsonParser::p_mode)) != NULL){
			cfg.outData.mode = (Blob::LightOutModeFlags)obj->valueint;
			keys |= Blob::LightKeyCfgOutm;
		}
		if((obj = cJSON_GetObjectItem(outData, JsonParser::p_numActions)) != NULL){
			cfg.outData.numActions = obj->valueint;
			cfg.outData.numActions = (cfg.outData.numActions > Blob::MaxAllowedActionDataInArray)? Blob::MaxAllowedActionDataInArray : cfg.outData.numActions;
		}

		if((value = cJSON_GetObjectItem(outData, JsonParser::p_curve)) != NULL){
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_samples)) != NULL){
				cfg.outData.curve.samples = obj->valueint;
				cfg.outData.curve.samples = (cfg.outData.curve.samples > Blob::LightCurveSampleCount)? Blob::LightCurveSampleCount : cfg.outData.curve.samples;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_data)) != NULL){
				if(cfg.outData.curve.samples == cJSON_GetArraySize(obj)){
					for(int i=0;i<cfg.outData.curve.samples;i++){
						item = cJSON_GetArrayItem(obj, i);
						cfg.outData.curve.data[i] = item->valueint;
					}
					keys |= Blob::LightKeyCfgCurve;
				}
			}
		}

		if((array = cJSON_GetObjectItem(outData, JsonParser::p_actions)) != NULL){
			if(cfg.outData.numActions == cJSON_GetArraySize(array)){
				for(int i=0;i<cfg.outData.numActions;i++){
					value = cJSON_GetArrayItem(array, i);
					if((obj = cJSON_GetObjectItem(value, JsonParser::p_id)) != NULL){
						cfg.outData.actions[i].id = obj->valueint;
					}
					if((obj = cJSON_GetObjectItem(value, JsonParser::p_flags)) != NULL){
						cfg.outData.actions[i].flags = (Blob::LightActionFlags)obj->valueint;
					}
					if((obj = cJSON_GetObjectItem(value, JsonParser::p_date)) != NULL){
						cfg.outData.actions[i].date = obj->valueint;
					}
					if((obj = cJSON_GetObjectItem(value, JsonParser::p_time)) != NULL){
						cfg.outData.actions[i].time = obj->valueint;
					}
					if((obj = cJSON_GetObjectItem(value, JsonParser::p_astCorr)) != NULL){
						cfg.outData.actions[i].astCorr = obj->valueint;
					}
					if((obj = cJSON_GetObjectItem(value, JsonParser::p_outValue)) != NULL){
						cfg.outData.actions[i].outValue = obj->valueint;
					}

					if((luxLevel = cJSON_GetObjectItem(value, JsonParser::p_luxLevel)) != NULL){
						if((obj = cJSON_GetObjectItem(luxLevel, JsonParser::p_min)) != NULL){
							cfg.outData.actions[i].luxLevel.min = obj->valueint;
						}
						if((obj = cJSON_GetObjectItem(luxLevel, JsonParser::p_max)) != NULL){
							cfg.outData.actions[i].luxLevel.max = obj->valueint;
						}
						if((obj = cJSON_GetObjectItem(luxLevel, JsonParser::p_thres)) != NULL){
							cfg.outData.actions[i].luxLevel.thres = obj->valueint;
						}
					}
				}
				keys |= Blob::LightKeyCfgActs;
			}
		}
	}
	return keys;
}


//------------------------------------------------------------------------------------
uint32_t getLightStatFromJson(Blob::LightStatData_t &stat, cJSON* json){
	cJSON *obj = NULL;
	if((obj = cJSON_GetObjectItem(json, JsonParser::p_flags)) == NULL){
		return 0;
	}
	stat.flags = obj->valueint;

	// key: outValue
	if((obj = cJSON_GetObjectItem(json, JsonParser::p_outValue)) == NULL){
		return 0;
	}
	stat.outValue = obj->valueint;

	return 1;
}
//------------------------------------------------------------------------------------
uint32_t getLightBootFromJson(Blob::LightBootData_t &obj, cJSON* json){
	cJSON* cfg = NULL;
	cJSON* stat = NULL;
	uint32_t keys = 0;

	if(json == NULL){
		return 0;
	}
	if((cfg = cJSON_GetObjectItem(json, JsonParser::p_cfg)) != NULL){
		keys |= getLightCfgFromJson(obj.cfg, cfg);
	}
	if((stat = cJSON_GetObjectItem(json, JsonParser::p_stat)) != NULL){
		keys |= getLightStatFromJson(obj.stat, stat);
	}
	return keys;
}

//------------------------------------------------------------------------------------
uint32_t getLightLuxFromJson(Blob::LightLuxLevel &lux, cJSON* json){
	cJSON *obj = NULL;
	// key: luxLevel
	if((obj = cJSON_GetObjectItem(json, JsonParser::p_luxLevel)) == NULL){
		return 0;
	}
	lux = obj->valueint;
	return 1;
}


//------------------------------------------------------------------------------------
uint32_t getLightTimeFromJson(Blob::AstCalStatData_t &obj, cJSON* json){
	return JSON::getAstCalStatFromJson(obj, json);
}

}	// end namespace JSON

