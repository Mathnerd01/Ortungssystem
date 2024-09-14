import math
import numpy as np

class ESP32:
    rssi_reference = -50
    path_loss_exponent = 2.7 
    
    def __init__(self, x, y ):
        self.x = x 
        self.y = y
        self.avg_rss = None
        self.abstand = None
    
    def update_rssi(self, avg_rss):
        self.avg_rss = avg_rss
        self.abstand = 10 ** ((self.rssi_reference - self.avg_rss) / (10 * self.path_loss_exponent))

esp32_1 = ESP32(0, 0)
esp32_2 = ESP32(10, 0)
esp32_3 = ESP32(5, 8.66)
esp32_4 = ESP32(0, 10)
esp32_5 = ESP32(10, 10)
esp32_6 = ESP32(20, 10)

# Example of updating RSSI 
esp32_1.update_rssi(-65.12)
esp32_2.update_rssi(-66.08)
esp32_3.update_rssi(-64.79)

def trilaterate(x1, y1, r1, x2, y2, r2, x3, y3, r3):
    
    # Calculate the coefficients for the linear equations
    A = -2*x1 + 2*x2
    B = -2*y1 + 2*y2
    C = r1**2 - r2**2 - x1**2 + x2**2 - y1**2 + y2**2
    D = -2*x1 + 2*x3
    E = -2*y1 + 2*y3
    F = r1**2 - r3**2 - x1**2 + x3**2 - y1**2 + y3**2
    
    # Create matrices for solving the linear equations
    coeff_matrix = np.array([[A, B], [D, E]])
    const_matrix = np.array([C, F])
    
    try:
        # Solve the system of linear equations
        solution = np.linalg.solve(coeff_matrix, const_matrix)
        x, y = solution[0], solution[1]
        return x, y
    except np.linalg.LinAlgError:
        # Handle the case where the system is unsolvable (e.g., no unique solution)
        print("The system has no unique solution. The points may be collinear or the distances inconsistent.")
        return None



# Use the trilaterate function with the positions and computed distances
result = trilaterate(esp32_1.x, esp32_1.y, esp32_1.abstand,
                     esp32_2.x, esp32_2.y, esp32_2.abstand,
                     esp32_3.x, esp32_3.y, esp32_3.abstand)

# Convert np.float64 to Python float
result = (float(result[0]), float(result[1]))

print(f"Estimated Position: {result}")
print(f"{esp32_1.abstand}")