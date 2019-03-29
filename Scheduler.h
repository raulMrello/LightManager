/*
 * Scheduler.h
 *
 *  Created on: Jun 2018
 *      Author: raulMrello
 *
 *	Scheduler es el módulo encargado de gestionar la ejecución de los programas. La operativa es como sigue en función
 *	de los servicios que proporciona:
 *
 *		buildActionList: crea una lista ordenada de las acciones que cumplen los criterios de ejecución
 *		para el día, hora, periodo actual
 *
 *		getLastAction: obtiene la última acción de la lista ordenada que se ejecutó o NULL si no hay
 *
 *		getNewAction: obtiene la acción que se debe ejecutar justo a la hora actual
 *
 *		findCurrAction: Busca la acción que debería estar en ejecución y hace un backtest en el rango de
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

    
    /** Crea un gestor de ejecución de acciones
     * 	@param action_count Número máximo de acciones gestionables por scheduler
     * 	@param actions Puntero al array de acciones
     * 	@param fs Objeto FSManager para operaciones de backup
     * 	@param defdbg Flag para habilitar depuración por defecto
     */
    Scheduler(uint8_t action_count, Blob::LightAction_t* actions, FSManager* fs, bool defdbg = false);


    /** Destructor */
    ~Scheduler(){}


    /** Actualiza el valor del luxómetro
     *
     * @param lux Iluminación
     * @return 0..100:Nuevo estado de la carga, -1:No hay acciones a ejecutar
     */
    int8_t updateLux(Blob::LightLuxLevel lux);


    /** Actualiza el timestamp actual
     *
     * @param ast Estado del calendario
     * @return 0..100:Nuevo estado de la carga, -1:No hay acciones a ejecutar
     */
    int8_t updateTimestamp(const Blob::LightTimeData_t& ast);


    /** Añade una nueva acción en memoria
     * 	@param pos Posición en la que añadir la acción
     *  @param action Acción a añadir.
     *  @return Resultado 0:Ok, <0:error
     */
    int32_t setAction(uint8_t pos, Blob::LightAction_t& action);


    /** Elimina una acción por su posición
     *  @param pos Posición de la maniobra a borrar
     *  @return Resultado 0:Ok, <0:error
     */
    int32_t clrActionByPos(int8_t pos);


    /** Elimina una o varias acciones por <id>
     *  @param id Id de la acción/acciones a borrar
     *  @return Resultado 0:Ok, <0:error
     */
    int32_t clrActionById(int8_t id);


    /** Elimina todas las acciones
     */
    void clrActions();


    /** Obtiene el número de acciones
     *  @return Acciones no vacías
     */
    uint8_t getActionCount();


    /** Recupera la lista de acciones del sistema de backup
     * 	@return True si recupera todos los parámetros.
     */
    bool restoreActionList();


    /** Guarda la lista de acciones en el sistema de backup
     *  @return True si se graba de forma correcta
     */
    bool saveActionList();


    /** Valida la información recuperada de las acciones
     * 	@return True si recupera correctamente la información
     */
    bool checkIntegrity();


    /** Obtiene la acción que debe ejecutarse en el instante actual
     *  @param filter Flags con los criterios de búsqueda (máscara de días, periodos, etc...)
     *  @param ast_data Datos para obtener los criterios de ejecución actual (hora, fecha, periodo...)
     *  @return Acción en ejecución o NULL si no hay acciones que cumplan el criterio
     */
    Blob::LightAction_t* findCurrAction(Blob::LightActionFlags filter, const Blob::LightTimeData_t& data);


    /**
     * Establece el nivel de las trazas de depuración
     * @param verbosity Nivel de depuración
     */
    void setVerbosity(esp_log_level_t verbosity);

private:

    /** Puntero al array de acciones */
    Blob::LightAction_t* _action_list;

    /** Número máximo de acciones */
    const uint8_t _max_action_count;

    /** Parámetros de ejecución */
    Blob::LightTimeData_t _ast_data;
    Blob::LightLuxLevel _lux;

    /** Parámetros de búsqueda y filtrado */
    Blob::LightActionFlags _filter;
    
    /**Gestor del sistema de backup */
    FSManager* _fs;

    /** Flag de depuración */
    const bool _defdbg;

};
     
#endif /*__Scheduler__H */

/**** END OF FILE ****/


