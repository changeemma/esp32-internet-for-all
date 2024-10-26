import subprocess
import csv
import json
import time
import os
from datetime import datetime

def run_iperf3_test(server_ip, port=5201, duration=10, parallel=1, reverse=False):
    """
    Ejecuta una prueba de iperf3 y retorna los resultados
    
    Args:
        server_ip (str): IP del servidor iperf3
        port (int): Puerto del servidor
        duration (int): Duración de la prueba en segundos
        parallel (int): Número de conexiones paralelas
        reverse (bool): True para prueba de download, False para upload
    """
    try:
        cmd = [
            'iperf3',
            '-c', server_ip,
            '-p', str(port),
            '-t', str(duration),
            '-P', str(parallel),
            '-J'  # Formato JSON
        ]
        
        if reverse:
            cmd.append('-R')
            
        result = subprocess.run(cmd, capture_output=True, text=True)
        return json.loads(result.stdout)
    except Exception as e:
        print(f"Error en la prueba: {e}")
        return None

def parse_results(results, test_id):
    """
    Extrae métricas relevantes del resultado de iperf3
    """
    if not results or 'end' not in results:
        return None
        
    try:
        end_info = results['end']
        sender = end_info['sum_sent']
        receiver = end_info['sum_received']
        
        return {
            'test_id': test_id,
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
            'bytes_transferred': receiver['bytes'],
            'bandwidth_mbps': receiver['bits_per_second'] / 1_000_000,
            'jitter_ms': receiver.get('jitter_ms', 0),
            'lost_packets': receiver.get('lost_packets', 0),
            'lost_percent': receiver.get('lost_percent', 0),
            'retransmits': sender.get('retransmits', 0)
        }
    except Exception as e:
        print(f"Error procesando resultados: {e}")
        return None

def get_last_test_id(filename):
    """
    Obtiene el último test_id del archivo CSV existente
    """
    if not os.path.exists(filename):
        return 0
    
    try:
        with open(filename, 'r') as csvfile:
            reader = csv.DictReader(csvfile)
            rows = list(reader)
            if rows:
                return max(int(row['test_id']) for row in rows)
    except Exception as e:
        print(f"Error leyendo archivo existente: {e}")
    return 0

def run_performance_tests(server_ip, num_tests=5, output_file='network_performance.csv'):
    """
    Ejecuta múltiples pruebas y agrega los resultados al archivo CSV
    """
    fieldnames = ['test_id', 'timestamp', 'bytes_transferred', 'bandwidth_mbps', 
                 'jitter_ms', 'lost_packets', 'lost_percent', 'retransmits', 'direction']
    
    # Obtener el último test_id
    last_test_id = get_last_test_id(output_file)
    current_test_id = last_test_id + 1
    
    results = []
    file_exists = os.path.exists(output_file)
    
    print(f"Iniciando pruebas de rendimiento (test_id inicial: {current_test_id})...")
    
    # Ejecutar pruebas
    for i in range(num_tests):
        print(f"\nEjecutando prueba {i+1}/{num_tests}")
        
        # Prueba de upload
        up_result = run_iperf3_test(server_ip, reverse=False)
        if up_result:
            parsed = parse_results(up_result, current_test_id)
            if parsed:
                parsed['direction'] = 'upload'
                results.append(parsed)
        
        # Esperar entre pruebas
        time.sleep(2)
        
        # Prueba de download
        down_result = run_iperf3_test(server_ip, reverse=True)
        if down_result:
            parsed = parse_results(down_result, current_test_id)
            if parsed:
                parsed['direction'] = 'download'
                results.append(parsed)
        
        current_test_id += 1
        time.sleep(2)
    
    # Guardar resultados en CSV
    mode = 'a' if file_exists else 'w'
    with open(output_file, mode, newline='') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        if not file_exists:
            writer.writeheader()
        writer.writerows(results)
    
    print(f"\nPruebas completadas. Resultados agregados a {output_file}")
    
    # Mostrar resumen de las nuevas pruebas
    print("\nResumen de resultados de esta sesión:")
    up_bw = [r['bandwidth_mbps'] for r in results if r['direction'] == 'upload']
    down_bw = [r['bandwidth_mbps'] for r in results if r['direction'] == 'download']
    
    if up_bw:
        print(f"Upload promedio: {sum(up_bw)/len(up_bw):.2f} Mbps")
    if down_bw:
        print(f"Download promedio: {sum(down_bw)/len(down_bw):.2f} Mbps")

def show_historical_summary(filename='network_performance.csv'):
    """
    Muestra un resumen de todas las pruebas almacenadas
    """
    if not os.path.exists(filename):
        print("No hay datos históricos disponibles.")
        return
        
    with open(filename, 'r') as csvfile:
        reader = csv.DictReader(csvfile)
        data = list(reader)
        
    up_bw = [float(row['bandwidth_mbps']) for row in data if row['direction'] == 'upload']
    down_bw = [float(row['bandwidth_mbps']) for row in data if row['direction'] == 'download']
    
    print("\nResumen histórico:")
    print(f"Total de pruebas: {len(data)}")
    if up_bw:
        print(f"Upload - Promedio: {sum(up_bw)/len(up_bw):.2f} Mbps")
        print(f"Upload - Máximo: {max(up_bw):.2f} Mbps")
        print(f"Upload - Mínimo: {min(up_bw):.2f} Mbps")
    if down_bw:
        print(f"Download - Promedio: {sum(down_bw)/len(down_bw):.2f} Mbps")
        print(f"Download - Máximo: {max(down_bw):.2f} Mbps")
        print(f"Download - Mínimo: {min(down_bw):.2f} Mbps")

if __name__ == "__main__":
    # Configuración de la prueba
    SERVER_IP = "192.170.85.2"  # Cambiar a la IP de tu servidor iperf3
    NUM_TESTS = 1  # Número de pruebas a realizar
    OUTPUT_FILE = "network_performance.csv"
    
    # Ejecutar nuevas pruebas
    run_performance_tests(SERVER_IP, NUM_TESTS, OUTPUT_FILE)
    
    # Mostrar resumen histórico
    show_historical_summary(OUTPUT_FILE)