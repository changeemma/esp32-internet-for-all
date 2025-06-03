#!/bin/bash

# Script para abrir 5 terminales y ejecutar comandos para ESP32
# Cada terminal ejecutará get_idf y luego flash en un puerto diferente (ttyUSB0-4)
# Se generará un archivo de log para registrar los resultados

# Directorio del proyecto
PROJECT_DIR="/home/juan/Documents/fiuba/esp32-internet-for-all"

# Archivo de log
LOG_FILE="$PROJECT_DIR/flash_results.log"

# Inicializar el archivo de log
echo "=== ESP32 Flash Results Log - $(date) ===" > "$LOG_FILE"
echo "" >> "$LOG_FILE"

# Verifica si el directorio existe
if [ ! -d "$PROJECT_DIR" ]; then
    echo "El directorio $PROJECT_DIR no existe. Por favor verifica la ruta." | tee -a "$LOG_FILE"
    exit 1
fi

# Verifica qué terminal está disponible en el sistema
if command -v gnome-terminal &> /dev/null; then
    TERMINAL="gnome-terminal"
elif command -v xterm &> /dev/null; then
    TERMINAL="xterm"
elif command -v konsole &> /dev/null; then
    TERMINAL="konsole"
elif command -v xfce4-terminal &> /dev/null; then
    TERMINAL="xfce4-terminal"
else
    echo "No se encontró un terminal compatible. Por favor instala gnome-terminal, xterm, konsole o xfce4-terminal." | tee -a "$LOG_FILE"
    exit 1
fi

# Función para abrir una terminal con los comandos específicos
open_terminal() {
    local usb_num=$1
    local port="/dev/ttyUSB$usb_num"
    local title="ESP32 - Puerto $port"
    local log_temp="$PROJECT_DIR/temp_log_$usb_num.log"
    
    # Mensaje inicial en el log
    echo "Iniciando operaciones para $port..." >> "$LOG_FILE"
    
    case $TERMINAL in
        "gnome-terminal")
            gnome-terminal --title="$title" -- zsh -c "cd \"$PROJECT_DIR\"; echo 'Configurando entorno para $port...'; source ~/.zshrc 2>&1 | tee \"$log_temp\"; echo 'Ejecutando get_idf para $port...'; get_idf 2>&1 | tee -a \"$log_temp\"; GET_IDF_STATUS=\$?; echo \"\nget_idf terminó con código: \$GET_IDF_STATUS\" | tee -a \"$log_temp\"; if [ \$GET_IDF_STATUS -eq 0 ]; then echo 'Configuración ESP-IDF completada. Ejecutando flash...'; idf.py -p $port flash 2>&1 | tee -a \"$log_temp\"; FLASH_STATUS=\$?; echo \"\nidf.py flash terminó con código: \$FLASH_STATUS\" | tee -a \"$log_temp\"; else echo 'Error en get_idf. No se ejecutará flash.' | tee -a \"$log_temp\"; fi; echo \"\n===== RESULTADOS PARA $port =====\" >> \"$LOG_FILE\"; cat \"$log_temp\" >> \"$LOG_FILE\"; echo \"\n\" >> \"$LOG_FILE\"; echo -e '\nProceso terminado. Presiona Enter para cerrar...'; read; rm \"$log_temp\""
            ;;
        "xterm")
            xterm -T "$title" -e "zsh -c \"cd \\\"$PROJECT_DIR\\\"; echo 'Configurando entorno para $port...'; source ~/.zshrc 2>&1 | tee \\\"$log_temp\\\"; echo 'Ejecutando get_idf para $port...'; get_idf 2>&1 | tee -a \\\"$log_temp\\\"; GET_IDF_STATUS=\\\$?; echo \\\"\\nget_idf terminó con código: \\\$GET_IDF_STATUS\\\" | tee -a \\\"$log_temp\\\"; if [ \\\$GET_IDF_STATUS -eq 0 ]; then echo 'Configuración ESP-IDF completada. Ejecutando flash...'; idf.py -p $port flash 2>&1 | tee -a \\\"$log_temp\\\"; FLASH_STATUS=\\\$?; echo \\\"\\nidf.py flash terminó con código: \\\$FLASH_STATUS\\\" | tee -a \\\"$log_temp\\\"; else echo 'Error en get_idf. No se ejecutará flash.' | tee -a \\\"$log_temp\\\"; fi; echo \\\"\\n===== RESULTADOS PARA $port =====\\\" >> \\\"$LOG_FILE\\\"; cat \\\"$log_temp\\\" >> \\\"$LOG_FILE\\\"; echo \\\"\\n\\\" >> \\\"$LOG_FILE\\\"; echo -e '\\nProceso terminado. Presiona Enter para cerrar...'; read; rm \\\"$log_temp\\\"\""
            ;;
        "konsole")
            konsole --new-tab -p tabtitle="$title" -e zsh -c "cd \"$PROJECT_DIR\"; echo 'Configurando entorno para $port...'; source ~/.zshrc 2>&1 | tee \"$log_temp\"; echo 'Ejecutando get_idf para $port...'; get_idf 2>&1 | tee -a \"$log_temp\"; GET_IDF_STATUS=\$?; echo \"\nget_idf terminó con código: \$GET_IDF_STATUS\" | tee -a \"$log_temp\"; if [ \$GET_IDF_STATUS -eq 0 ]; then echo 'Configuración ESP-IDF completada. Ejecutando flash...'; idf.py -p $port flash 2>&1 | tee -a \"$log_temp\"; FLASH_STATUS=\$?; echo \"\nidf.py flash terminó con código: \$FLASH_STATUS\" | tee -a \"$log_temp\"; else echo 'Error en get_idf. No se ejecutará flash.' | tee -a \"$log_temp\"; fi; echo \"\n===== RESULTADOS PARA $port =====\" >> \"$LOG_FILE\"; cat \"$log_temp\" >> \"$LOG_FILE\"; echo \"\n\" >> \"$LOG_FILE\"; echo -e '\nProceso terminado. Presiona Enter para cerrar...'; read; rm \"$log_temp\""
            ;;
        "xfce4-terminal")
            xfce4-terminal --title="$title" --command="zsh -c 'cd \"$PROJECT_DIR\"; echo \"Configurando entorno para $port...\"; source ~/.zshrc 2>&1 | tee \"$log_temp\"; echo \"Ejecutando get_idf para $port...\"; get_idf 2>&1 | tee -a \"$log_temp\"; GET_IDF_STATUS=\$?; echo \"\nget_idf terminó con código: \$GET_IDF_STATUS\" | tee -a \"$log_temp\"; if [ \$GET_IDF_STATUS -eq 0 ]; then echo \"Configuración ESP-IDF completada. Ejecutando flash...\"; idf.py -p $port flash 2>&1 | tee -a \"$log_temp\"; FLASH_STATUS=\$?; echo \"\nidf.py flash terminó con código: \$FLASH_STATUS\" | tee -a \"$log_temp\"; else echo \"Error en get_idf. No se ejecutará flash.\" | tee -a \"$log_temp\"; fi; echo \"\n===== RESULTADOS PARA $port =====\" >> \"$LOG_FILE\"; cat \"$log_temp\" >> \"$LOG_FILE\"; echo \"\n\" >> \"$LOG_FILE\"; echo -e \"\nProceso terminado. Presiona Enter para cerrar...\"; read; rm \"$log_temp\"'"
            ;;
    esac
    
    # Pequeña pausa para evitar sobrecarga al abrir múltiples terminales
    sleep 1
}

# Abre 5 terminales con los comandos correspondientes
for i in {0..4}; do
    open_terminal $i
done

echo "Se han abierto 5 terminales para los puertos ttyUSB0 a ttyUSB4. El log se guardará en $LOG_FILE"