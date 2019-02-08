/*
 * test_LightManager.cpp
 *
 *	Test unitario para el módulo LightManager
 */


//------------------------------------------------------------------------------------
//-- TEST HEADERS --------------------------------------------------------------------
//------------------------------------------------------------------------------------

#include "unity.h"
#include "LightManager.h"
#include "JsonParserBlob.h"

//------------------------------------------------------------------------------------
//-- REQUIRED HEADERS & COMPONENTS FOR TESTING ---------------------------------------
//------------------------------------------------------------------------------------

#include "AppConfig.h"
#include "FSManager.h"
#include "cJSON.h"

/** variables requeridas para realizar el test */
static FSManager* fs=NULL;
static MQ::PublishCallback s_published_cb;
static void subscriptionCb(const char* topic, void* msg, uint16_t msg_len);
static void publishedCb(const char* topic, int32_t result);
static void executePrerequisites();
static bool s_test_done = false;


//------------------------------------------------------------------------------------
//-- SPECIFIC COMPONENTS FOR TESTING -------------------------------------------------
//------------------------------------------------------------------------------------

static LightManager* light=NULL;
static const char* _MODULE_ = "[TEST_Light]....";
#define _EXPR_	(true)



//------------------------------------------------------------------------------------
//-- TEST CASES ----------------------------------------------------------------------
//------------------------------------------------------------------------------------

//---------------------------------------------------------------------------
/**
 * @brief Se verifica la creación del objeto y la suscripción a topics
 * MQLib
 */
TEST_CASE("Init & MQLib suscription..............", "[LightManager]") {

	// ejecuta requisitos previos
	executePrerequisites();

	// crea el objeto
	TEST_ASSERT_NULL(light);
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Iniciando LightManager con driver 010... ");
	light = new LightManager(GPIO_NUM_26, fs, true);
	TEST_ASSERT_NOT_NULL(light);

	light->setPublicationBase("light");
	light->setSubscriptionBase("light");
	do{
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Esperando a LightManager");
		Thread::wait(100);
	}while(!light->ready());
	TEST_ASSERT_TRUE(light->ready());
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "LightManager OK!");
}


//---------------------------------------------------------------------------
/**
 * @brief Se verifican las publicaciones y suscripciones JSON, para ello el módulo
 * LightManager debe ser compilado con la opción COMP_ENABLE_JSON_SUPPORT=1 o se debe
 * activar dicha capacidad mediante LightManager::setJSONSupport(true)
 */
TEST_CASE("JSON support .........................", "[LightManager]"){

	// activa soporte JSON
	light->setJSONSupport(true);
	TEST_ASSERT_TRUE(light->isJSONSupported());

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;

	// solicita la trama de arranque
	char* msg = "{}";
	MQ::ErrorResult res = MQ::MQClient::publish("get/boot/light", msg, strlen(msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

	// wait for response at least 10 seconds, yielding this thread
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
	double count = 0;
	do{
		Thread::wait(100);
		count += 0.1;
	}while(!s_test_done && count < 10);
	TEST_ASSERT_TRUE(s_test_done);

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;

	// solicita la configuración mediante un GetRequest
	Blob::GetRequest_t* greq = new Blob::GetRequest_t(1);
	TEST_ASSERT_NOT_NULL(greq);
	cJSON* jreq = JsonParser::getJsonFromObj(*greq);
	TEST_ASSERT_NOT_NULL(jreq);
	msg = cJSON_Print(jreq);
	TEST_ASSERT_NOT_NULL(msg);
	cJSON_Delete(jreq);
	delete(greq);

	res = MQ::MQClient::publish("get/cfg/light", msg, strlen(msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	Heap::memFree(msg);

	// wait for response at least 10 seconds, yielding this thread
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
	count = 0;
	do{
		Thread::wait(100);
		count += 0.1;
	}while(!s_test_done && count < 10);
	TEST_ASSERT_TRUE(s_test_done);

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;

	// solicita la configuración mediante un GetRequest
	greq = new Blob::GetRequest_t(2);
	TEST_ASSERT_NOT_NULL(greq);
	jreq = JsonParser::getJsonFromObj(*greq);
	TEST_ASSERT_NOT_NULL(jreq);
	msg = cJSON_Print(jreq);
	TEST_ASSERT_NOT_NULL(msg);
	cJSON_Delete(jreq);
	delete(greq);

	res = MQ::MQClient::publish("get/value/light", msg, strlen(msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	Heap::memFree(msg);

	// wait for response at least 10 seconds, yielding this thread
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
	count = 0;
	do{
		Thread::wait(100);
		count += 0.1;
	}while(!s_test_done && count < 10);
	TEST_ASSERT_TRUE(s_test_done);

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;

	// actualiza la configuración mediante un SetRequest
	Blob::SetRequest_t<Blob::LightCfgData_t> req;
	req.idTrans = 3;
	req.data.updFlagMask=1;
	req.data.evtFlagMask=7;
	req.data.alsData.lux = {200,1000,50};
	req.data.outData.mode=17;
	req.data.outData.curve.samples=11;
	uint8_t def_data[] = {7,15,20,23,30,35,40,45,51,57,60};
	for(int i=0;i<11;i++){
		req.data.outData.curve.data[i] = def_data[i];
	}
	req.data.outData.numActions=2;
	req.data.outData.actions[0].id=0;							//!< Identificador de la acción
	req.data.outData.actions[0].flags=524415;				//!< Flags de control de la acción a realizar
	req.data.outData.actions[0].date=0;						//!< Fecha ddMM para activar la acción
	req.data.outData.actions[0].time=1440; 						//!< Hora fija de activación (min. día)
	req.data.outData.actions[0].astCorr=0;						//!< Corrección sobre el hito astronómico en caso de estar habilitado
	req.data.outData.actions[0].luxLevel={0,300,50};				//!< Nivel de luminosidad a partir de la cual se activará
	req.data.outData.actions[0].outValue=100;
	req.data.outData.actions[1].id=0;							//!< Identificador de la acción
	req.data.outData.actions[1].flags=524415;				//!< Flags de control de la acción a realizar
	req.data.outData.actions[1].date=0;						//!< Fecha ddMM para activar la acción
	req.data.outData.actions[1].time=1440; 						//!< Hora fija de activación (min. día)
	req.data.outData.actions[1].astCorr=0;						//!< Corrección sobre el hito astronómico en caso de estar habilitado
	req.data.outData.actions[1].luxLevel={700,250000,50};				//!< Nivel de luminosidad a partir de la cual se activará
	req.data.outData.actions[1].outValue=0;

	jreq = JsonParser::getJsonFromSetRequest(req, JsonParser::p_data);
	TEST_ASSERT_NOT_NULL(jreq);
	msg = cJSON_Print(jreq);
	TEST_ASSERT_NOT_NULL(msg);
	cJSON_Delete(jreq);

	res = MQ::MQClient::publish("set/cfg/light", msg, strlen(msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	Heap::memFree(msg);

	// wait for response at least 10 seconds, yielding this thread
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
	count = 0;
	do{
		Thread::wait(100);
		count += 0.1;
	}while(!s_test_done && count < 10);
	TEST_ASSERT_TRUE(s_test_done);
}


//---------------------------------------------------------------------------
/**
 * @brief Se verifican las publicaciones y suscripciones JSON, para ello el módulo
 * LightManager debe ser compilado con la opción ASTCAL_ENABLE_JSON_SUPPORT=1 o se debe
 * activar dicha capacidad mediante LightManager::setJSONSupport(true)
 */
TEST_CASE("Blob support .........................", "[LightManager]"){

	// activa soporte JSON
	light->setJSONSupport(false);
	TEST_ASSERT_FALSE(light->isJSONSupported());

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;

	// solicita la trama de arranque
	char* msg = "{}";
	MQ::ErrorResult res = MQ::MQClient::publish("get/boot/light", msg, strlen(msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

	// wait for response at least 10 seconds, yielding this thread
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
	double count = 0;
	do{
		Thread::wait(100);
		count += 0.1;
	}while(!s_test_done && count < 10);
	TEST_ASSERT_TRUE(s_test_done);

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;

	// solicita la configuración mediante un GetRequest
	Blob::GetRequest_t greq;
	greq.idTrans = 4;
	greq._error.code = Blob::ErrOK;
	greq._error.descr[0] = 0;

	res = MQ::MQClient::publish("get/cfg/light", &greq, sizeof(Blob::GetRequest_t), &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);


	// wait for response at least 10 seconds, yielding this thread
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
	count = 0;
	do{
		Thread::wait(100);
		count += 0.1;
	}while(!s_test_done && count < 10);
	TEST_ASSERT_TRUE(s_test_done);

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;

	// solicita la configuración mediante un GetRequest
	greq.idTrans = 5;
	greq._error.code = Blob::ErrOK;
	greq._error.descr[0] = 0;

	res = MQ::MQClient::publish("get/value/light", &greq, sizeof(Blob::GetRequest_t), &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);


	// wait for response at least 10 seconds, yielding this thread
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
	count = 0;
	do{
		Thread::wait(100);
		count += 0.1;
	}while(!s_test_done && count < 10);
	TEST_ASSERT_TRUE(s_test_done);

	// -----------------------------------------------
	// borra flag de resultado
	s_test_done = false;

	// actualiza la configuración mediante un SetRequest
	Blob::SetRequest_t<Blob::LightCfgData_t> sreq;
	sreq.idTrans = 6;
	sreq.keys = Blob::LightKeyCfgAll;
	sreq._error.code = Blob::ErrOK;
	sreq._error.descr[0] = 0;
	sreq.data.updFlagMask=1;
	sreq.data.evtFlagMask=7;
	sreq.data.alsData.lux = {200,1000,50};
	sreq.data.outData.mode=17;
	sreq.data.outData.curve.samples=11;
	uint8_t def_data[] = {7,15,20,23,30,35,40,45,51,57,60};
	for(int i=0;i<11;i++){
		sreq.data.outData.curve.data[i] = def_data[i];
	}
	sreq.data.outData.numActions=2;
	sreq.data.outData.actions[0].id=0;							//!< Identificador de la acción
	sreq.data.outData.actions[0].flags=524415;				//!< Flags de control de la acción a realizar
	sreq.data.outData.actions[0].date=0;						//!< Fecha ddMM para activar la acción
	sreq.data.outData.actions[0].time=1440; 						//!< Hora fija de activación (min. día)
	sreq.data.outData.actions[0].astCorr=0;						//!< Corrección sobre el hito astronómico en caso de estar habilitado
	sreq.data.outData.actions[0].luxLevel={0,300,50};				//!< Nivel de luminosidad a partir de la cual se activará
	sreq.data.outData.actions[0].outValue=100;
	sreq.data.outData.actions[1].id=0;							//!< Identificador de la acción
	sreq.data.outData.actions[1].flags=524415;				//!< Flags de control de la acción a realizar
	sreq.data.outData.actions[1].date=0;						//!< Fecha ddMM para activar la acción
	sreq.data.outData.actions[1].time=1440; 						//!< Hora fija de activación (min. día)
	sreq.data.outData.actions[1].astCorr=0;						//!< Corrección sobre el hito astronómico en caso de estar habilitado
	sreq.data.outData.actions[1].luxLevel={700,250000,50};				//!< Nivel de luminosidad a partir de la cual se activará
	sreq.data.outData.actions[1].outValue=0;

	res = MQ::MQClient::publish("set/cfg/light", &sreq, sizeof(Blob::SetRequest_t<Blob::LightCfgData_t>), &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

	// wait for response at least 10 seconds, yielding this thread
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Wait 10secs to get a response...");
	count = 0;
	do{
		Thread::wait(100);
		count += 0.1;
	}while(!s_test_done && count < 10);
	TEST_ASSERT_TRUE(s_test_done);
}

//------------------------------------------------------------------------------------
//-- PREREQUISITES -------------------------------------------------------------------
//------------------------------------------------------------------------------------


/** Prerequisites execution control flag */
static bool s_executed_prerequisites = false;


//------------------------------------------------------------------------------------
static void publishedCb(const char* topic, int32_t result){

}


//------------------------------------------------------------------------------------
static void subscriptionCb(const char* topic, void* msg, uint16_t msg_len){
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Recibido topic %s con mensaje:", topic);
	cJSON* obj = cJSON_Parse(msg);
	// Print JSON object
	if(obj){
		cJSON_Delete(obj);
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "%s", (char*)msg);
	}
	// Decode depending on the topic
	else{
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Formando objeto JSON a partir de objeto Blob...");
		if(MQ::MQClient::isTokenRoot(topic, "stat/cfg")){
			if(msg_len == sizeof(Blob::Response_t<Blob::LightCfgData_t>)){
				cJSON* obj = JsonParser::getJsonFromResponse(*((Blob::Response_t<Blob::LightCfgData_t>*)msg));
				if(obj){
					char* sobj = cJSON_Print(obj);
					cJSON_Delete(obj);
					DEBUG_TRACE_I(_EXPR_, _MODULE_, "%s", sobj);
					Heap::memFree(sobj);
				}
			}
			else if(msg_len == sizeof(Blob::LightCfgData_t)){
				cJSON* obj = JsonParser::getJsonFromObj(*((Blob::LightCfgData_t*)msg));
				if(obj){
					char* sobj = cJSON_Print(obj);
					cJSON_Delete(obj);
					DEBUG_TRACE_I(_EXPR_, _MODULE_, "%s", sobj);
					Heap::memFree(sobj);
				}
			}

		}
		else if(MQ::MQClient::isTokenRoot(topic, "stat/value")){
			if(msg_len == sizeof(Blob::Response_t<Blob::LightStatData_t>)){
				cJSON* obj = JsonParser::getJsonFromResponse(*((Blob::Response_t<Blob::LightStatData_t>*)msg));
				if(obj){
					char* sobj = cJSON_Print(obj);
					cJSON_Delete(obj);
					DEBUG_TRACE_I(_EXPR_, _MODULE_, "%s", sobj);
					Heap::memFree(sobj);
				}
			}
			else if(msg_len == sizeof(Blob::LightStatData_t)){
				cJSON* obj = JsonParser::getJsonFromObj(*((Blob::LightStatData_t*)msg));
				if(obj){
					char* sobj = cJSON_Print(obj);
					cJSON_Delete(obj);
					DEBUG_TRACE_I(_EXPR_, _MODULE_, "%s", sobj);
					Heap::memFree(sobj);
				}
			}
		}
		else if(MQ::MQClient::isTokenRoot(topic, "stat/boot")){
			if(msg_len == sizeof(Blob::LightBootData_t)){
				Blob::LightBootData_t* boot = (Blob::LightBootData_t*)msg;
				cJSON* obj = JsonParser::getJsonFromObj(*((Blob::LightBootData_t*)msg));
				if(obj){
					char* sobj = cJSON_Print(obj);
					cJSON_Delete(obj);
					DEBUG_TRACE_I(_EXPR_, _MODULE_, "%s", sobj);
					Heap::memFree(sobj);
				}
			}
		}
		else{
			DEBUG_TRACE_I(_EXPR_, _MODULE_, "Error procesando mensaje en topic %s", topic);
			s_test_done = false;
			return;
		}
	}
	s_test_done = true;
}


//------------------------------------------------------------------------------------
static void executePrerequisites(){
	if(!s_executed_prerequisites){

		// inicia mqlib
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Init MQLib...");
		MQ::ErrorResult res = MQ::MQBroker::start(64);
		TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

		// espera a que esté disponible
		while(!MQ::MQBroker::ready()){
			Thread::wait(100);
		}
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "MQLib OK!");

		// registra un manejador de publicaciones común
		s_published_cb = callback(&publishedCb);

		// se suscribe a todas las notificaciones de estado: stat/#
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Suscription to stat/#");
		res = MQ::MQClient::subscribe("stat/#", new MQ::SubscribeCallback(&subscriptionCb));
		TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

		// inicia el subsistema NV
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Init FSManager... ");

		fs = new FSManager("fs");
		TEST_ASSERT_NOT_NULL(fs);
		while(!fs->ready()){
			Thread::wait(100);
		}
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "FSManager OK!");

		// marca flag de prerequisitos completado
		s_executed_prerequisites = true;
	}
}


