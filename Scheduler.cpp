/*
 * Scheduler.cpp
 *
 *  Created on: 20/04/2015
 *      Author: raulMrello
 */

#include "Scheduler.h"



//------------------------------------------------------------------------------------
//-- PRIVATE TYPEDEFS ----------------------------------------------------------------
//------------------------------------------------------------------------------------

/** Macro para imprimir trazas de depuración, siempre que se haya configurado un objeto
 *	Logger válido (ej: _debug)
 */
static const char* _MODULE_ = "[RlyMan]........";
#define _EXPR_	(_defdbg && !IS_ISR())


//------------------------------------------------------------------------------------
static Blob::LightActionFlags weekDayFlagsFromTM(int wday){
	switch(wday){
		case 0:
			return Blob::LightActionSun;
		case 1:
			return Blob::LightActionMon;
		case 2:
			return Blob::LightActionTue;
		case 3:
			return Blob::LightActionWed;
		case 4:
			return Blob::LightActionThr;
		case 5:
			return Blob::LightActionFri;
		default:
			return Blob::LightActionSat;
	}
}


//------------------------------------------------------------------------------------
static int32_t getExecutionTime(const Blob::LightAction_t* action, const Blob::AstCalStatData_t& ast_data){
	int32_t result = -1;
	tm now;
	localtime_r(&ast_data.now, &now);
	uint16_t minutes = (now.tm_hour * 60) + now.tm_min;
	uint16_t date = now.tm_mday * 128 + now.tm_mon;
	Blob::LightActionFlags flags = weekDayFlagsFromTM(now.tm_wday);

	// si la máscara de periodos no coincide, termina con error
	if((action->flags & (1 << ast_data.period)) == 0)
		return -1;

	// si la acción es a una fecha concreta y no coincide, termina
	if((action->flags & (Blob::LightActionFixDate)) && date != action->date)
		return -1;

	// si la acción es por días de la semana y no coincide
	if((action->flags & (Blob::LightActionSun|Blob::LightActionMon|Blob::LightActionTue|Blob::LightActionWed|Blob::LightActionThr|Blob::LightActionFri|Blob::LightActionSat)) && (action->flags & weekDayFlagsFromTM(now.tm_wday)) == 0)
		return -1;

	// en este punto, la acción es ejecutable hoy:

	// si es al orto...
	if(action->flags & Blob::LightActionDawn){
		result = ast_data.astData.wdowDawnStart + action->astCorr;
	}
	// si es al ocaso...
	else if(action->flags & Blob::LightActionDusk){
		result = ast_data.astData.wdowDuskStart + action->astCorr;
	}
	// si no, es que es a una hora fija
	else if(action->flags & (Blob::LightActionFixTime)){
		result = action->time;
	}

	return result;
}

 
//------------------------------------------------------------------------------------
//-- PUBLIC METHODS IMPLEMENTATION ---------------------------------------------------
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------
Scheduler::Scheduler(uint8_t action_count, Blob::LightAction_t* actions,FSManager* fs, bool defdbg) : _max_action_count(action_count), _fs(fs), _defdbg(defdbg) {
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Creando objeto");

    // borro propiedades a sus valores por defecto
    memset(&_ast_data, 0, sizeof(Blob::AstCalStatData_t));
    _filter = Blob::LightNoActions;
    _lux = 0;
    
    _action_list = actions;

    DEBUG_TRACE_I(_EXPR_, _MODULE_, "Objeto listo!");
}


//------------------------------------------------------------------------------------
int32_t Scheduler::setAction(uint8_t pos, Blob::LightAction_t& action){
	if(pos >= _max_action_count)
		return -1;

	_action_list[pos] = action;
	return 0;
}


//------------------------------------------------------------------------------------
int32_t Scheduler::clrActionByPos(int8_t pos){
	if(pos >= _max_action_count)
		return -1;

	_action_list[pos]={
			0, 						// id: 		no se utiliza
			Blob::LightNoActions,	// flags: 	Sin acción asignada
			0,						// date:
			0,						// time:
			0,						// astCorr:
			{0,0,0},				// luxLevel:
			-1};					// outValue: -1 Acción desactivada
	return 0;
}


//------------------------------------------------------------------------------------
int32_t Scheduler::clrActionById(int8_t id){
	for(int i=0;i<_max_action_count;i++){
		if(_action_list[i].id == id){
			_action_list[i]={
						0, 						// id: 		no se utiliza
						Blob::LightNoActions,	// flags: 	Sin acción asignada
						0,						// date:
						0,						// time:
						0,						// astCorr:
						{0,0,0},				// luxLevel:
						-1};					// outValue: -1 Acción desactivada
		}
	}
	return 0;
}


//------------------------------------------------------------------------------------
void Scheduler::clrActions(){
	for(int i=0;i<_max_action_count;i++){
		_action_list[i]={
					0, 						// id: 		no se utiliza
					Blob::LightNoActions,	// flags: 	Sin acción asignada
					0,						// date:
					0,						// time:
					0,						// astCorr:
					{0,0,0},				// luxLevel:
					-1};					// outValue: -1 Acción desactivada
	}
}


//------------------------------------------------------------------------------------
uint8_t Scheduler::getActionCount(){
	uint8_t count = 0;
	for(int i=0;i<_max_action_count;i++){
		if(_action_list[i].id != 0){
			count++;
		}
	}
	return count;
}


//------------------------------------------------------------------------------------
bool Scheduler::restoreActionList(){
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Recuperando datos de memoria NV...");
	bool result = true;
	for(int i=0;i<_max_action_count;i++){
		char paramId[strlen("SchAct_XX")];
		sprintf(paramId, "SchAct_%d", i);
		if(!_fs->restore(paramId, &_action_list[i], sizeof(Blob::LightAction_t), NVSInterface::TypeBlob)){
			DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS leyendo SchAction_%d!", i);
			result = false;
		}
	}
	return result;
}


//------------------------------------------------------------------------------------
bool Scheduler::saveActionList(){
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Guardando datos en memoria NV...");
	bool result = true;
	for(int i=0;i<_max_action_count;i++){
		char paramId[strlen("SchAction_XX")];
		sprintf(paramId, "SchAction_%d", i);
		if(!_fs->save(paramId, &_action_list[i], sizeof(Blob::LightAction_t), NVSInterface::TypeBlob)){
			DEBUG_TRACE_W(_EXPR_, _MODULE_, "ERR_NVS guardando accion i=%d", i);
			result = false;
		}
	}
	return result;
}


//------------------------------------------------------------------------------------
bool Scheduler::checkIntegrity(){
	for(int i=0;i<_max_action_count;i++){
		if((_action_list[i].id >= _max_action_count) ||
			   (_action_list[i].date < Blob::LightActionDateMin || _action_list[i].date > Blob::LightActionDateMax) ||
			   (_action_list[i].time > Blob::LightActionTimeMax) ||
			   (_action_list[i].outValue > Blob::LightActionOutMax)){
			return false;
		}
	}
	return true;
}


//------------------------------------------------------------------------------------
Blob::LightAction_t* Scheduler::findCurrAction(Blob::LightActionFlags filter, Blob::AstCalStatData_t& ast_data){
	tm now;
	localtime_r(&ast_data.now, &now);
	uint16_t curr_time = (now.tm_hour * 60) + now.tm_min;
	int32_t exec_time = -1;
	Blob::LightAction_t* curr = NULL;

	for(int i=0;i<_max_action_count;i++){
		if(_action_list[i].id != 0 && (_action_list[i].flags & filter) != 0){
			int32_t action_time = getExecutionTime(&_action_list[i], ast_data);
			if(action_time >= 0 && action_time <= curr_time && action_time > exec_time){
				curr = &_action_list[i];
				exec_time = action_time;
			}
		}
	}

	return curr;
}


//------------------------------------------------------------------------------------
int8_t Scheduler::updateLux(Blob::LightLuxLevel lux){
	int8_t result = -1;
	_lux = lux;
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Ejecutando scheduler con lux=%d", _lux);
	for(int i=0;i<_max_action_count;i++){
		// busca acciones asociadas al sensor de iluminación, que no estén en ejecución
		if(result == -1 && _action_list[i].id >= 0 && (_action_list[i].flags & (Blob::LightActionAls|Blob::LighActionAlsActive)) == Blob::LightActionAls){
			// si el nivel de luminosidad está en rango, las activa
			if(_lux >= _action_list[i].luxLevel.min && _lux <= _action_list[i].luxLevel.max){
				DEBUG_TRACE_D(_EXPR_, _MODULE_, "Prog id=%d, ejecutado por Lux=%d, out=%d", _action_list[i].id, _lux, _action_list[i].outValue);
				_action_list[i].flags = (Blob::LightActionFlags)(_action_list[i].flags | Blob::LighActionAlsActive);
				result = _action_list[i].outValue;
				continue;
			}
		}
		// busca acciones asociadas al sensor de iluminación, que estén en ejecución
		else if(_action_list[i].id >= 0 && (_action_list[i].flags & (Blob::LightActionAls|Blob::LighActionAlsActive)) == (Blob::LightActionAls|Blob::LighActionAlsActive)){
			// si el nivel de luminosidad está fuera de rango (+ threshold) , las desactiva
			if(_lux <= (_action_list[i].luxLevel.min-_action_list[i].luxLevel.thres) || _lux >= (_action_list[i].luxLevel.max+_action_list[i].luxLevel.thres)){
				DEBUG_TRACE_D(_EXPR_, _MODULE_, "Prog id=%d, sale de rango Lux=%d", _action_list[i].id, _lux);
				_action_list[i].flags = (Blob::LightActionFlags)(_action_list[i].flags & ~Blob::LighActionAlsActive);
				continue;
			}
		}
	}
	return result;
}


//------------------------------------------------------------------------------------
int8_t Scheduler::updateTimestamp(const Blob::AstCalStatData_t& ast){
	_ast_data = ast;
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Ejecutando scheduler con timestamp_flags=%x", _ast_data.flags);
	for(int i=0;i<_max_action_count;i++){
		// busca acciones asociadas a una hora fija...
		if(_action_list[i].id >= 0 && (_action_list[i].flags & Blob::LightActionFixTime) != 0){
			// si la hora coincide y los días de la semana también, se ejecuta
			tm now;
			localtime_r(&_ast_data.now, &now);
			uint16_t hhmm = now.tm_hour*60 + now.tm_min;
			if(hhmm == _action_list[i].time && (weekDayFlagsFromTM(now.tm_wday) & _action_list[i].flags) !=0){
				DEBUG_TRACE_D(_EXPR_, _MODULE_, "Prog id=%d, ejecutado por timestamp=%d, out=%d", _action_list[i].id, hhmm, _action_list[i].outValue);
				return _action_list[i].outValue;
			}
		}
	}
	return -1;
}


//------------------------------------------------------------------------------------
//-- PROTECTED METHODS IMPLEMENTATION ------------------------------------------------
//------------------------------------------------------------------------------------

