/*
 * Scheduler.h
 *
 *  Created on: Jun 2018
 *      Author: raulMrello
 *
 *	Scheduler es el m�dulo encargado de gestionar la ejecuci�n de los programas. La operativa es como sigue en funci�n
 *	de los servicios que proporciona:
 *
 *		buildActionList: crea una lista ordenada de las acciones que cumplen los criterios de ejecuci�n
 *		para el d�a, hora, periodo actual
 *
 *		getLastAction: obtiene la �ltima acci�n de la lista ordenada que se ejecut� o NULL si no hay
 *
 *		getNewAction: obtiene la acci�n que se debe ejecutar justo a la hora actual
 *
 *		findCurrAction: Busca la acci�n que deber�a estar en ejecuci�n y hace un backtest en el rango de
 *		fechas establecido
 *
 *
 */
 
#ifndef __Scheduler__H
#define __Scheduler__H

#include "mbed.h"
#include "LightManagerBlob.h"
#include "FSManager.h"
#include "List.h"
   
class Scheduler {
  public:

    
    /** Crea un gestor de ejecuci�n de acciones
     * 	@param action_count N�mero m�ximo de acciones gestionables por scheduler
     * 	@param actions Puntero al array de acciones
     * 	@param fs Objeto FSManager para operaciones de backup
     * 	@param defdbg Flag para habilitar depuraci�n por defecto
     */
    Scheduler(uint8_t action_count, Blob::LightAction_t* actions, FSManager* fs, bool defdbg = false);


    /** Destructor */
    ~Scheduler(){}


    /** Actualiza el valor del lux�metro
     *
     * @param lux Iluminaci�n
     * @return 0..100:Nuevo estado de la carga, -1:No hay acciones a ejecutar
     */
    int8_t updateLux(Blob::LightLuxLevel lux);


    /** Actualiza el timestamp actual
     *
     * @param ast Estado del calendario
     * @return 0..100:Nuevo estado de la carga, -1:No hay acciones a ejecutar
     */
    int8_t updateTimestamp(const Blob::LightTimeData_t& ast);


    /** A�ade una nueva acci�n en memoria
     * 	@param pos Posici�n en la que a�adir la acci�n
     *  @param action Acci�n a a�adir.
     *  @return Resultado 0:Ok, <0:error
     */
    int32_t setAction(uint8_t pos, Blob::LightAction_t& action);


    /** Elimina una acci�n por su posici�n
     *  @param pos Posici�n de la maniobra a borrar
     *  @return Resultado 0:Ok, <0:error
     */
    int32_t clrActionByPos(int8_t pos);


    /** Elimina una o varias acciones por <id>
     *  @param id Id de la acci�n/acciones a borrar
     *  @return Resultado 0:Ok, <0:error
     */
    int32_t clrActionById(int8_t id);


    /** Elimina todas las acciones
     */
    void clrActions();


    /** Obtiene el n�mero de acciones
     *  @return Acciones no vac�as
     */
    uint8_t getActionCount();


    /** Recupera la lista de acciones del sistema de backup
     * 	@return True si recupera todos los par�metros.
     */
    bool restoreActionList();


    /** Guarda la lista de acciones en el sistema de backup
     *  @return True si se graba de forma correcta
     */
    bool saveActionList();


    /** Valida la informaci�n recuperada de las acciones
     * 	@return True si recupera correctamente la informaci�n
     */
    bool checkIntegrity();


    /** Obtiene la acci�n que debe ejecutarse en el instante actual
     *  @param filter Flags con los criterios de b�squeda (m�scara de d�as, periodos, etc...)
     *  @param ast_data Datos para obtener los criterios de ejecuci�n actual (hora, fecha, periodo...)
     *  @return Acci�n en ejecuci�n o NULL si no hay acciones que cumplan el criterio
     */
    Blob::LightAction_t* findCurrAction(Blob::LightActionFlags filter, const Blob::LightTimeData_t& data);


    /**
     * Establece el nivel de las trazas de depuraci�n
     * @param verbosity Nivel de depuraci�n
     */
    void setVerbosity(esp_log_level_t verbosity);

private:

    /** Puntero al array de acciones */
    Blob::LightAction_t* _action_list;

    /** N�mero m�ximo de acciones */
    const uint8_t _max_action_count;

    /** Par�metros de ejecuci�n */
    Blob::LightTimeData_t _ast_data;
    Blob::LightLuxLevel _lux;

    /** Par�metros de b�squeda y filtrado */
    Blob::LightActionFlags _filter;
    
    /**Gestor del sistema de backup */
    FSManager* _fs;

    /** Flag de depuraci�n */
    const bool _defdbg;

};
     
#endif /*__Scheduler__H */

/**** END OF FILE ****/


