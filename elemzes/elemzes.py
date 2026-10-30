import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import find_peaks

print("--- Okoskaptár Akusztikus Elemzés ---")

#Adatok betöltése
df = pd.read_csv('adatok.csv')
df['idobelyeg'] = pd.to_datetime(df['idobelyeg'])
df.set_index('idobelyeg', inplace=True)

#Hibás mérések szűrése és dátum szerinti kiválasztás
df = df[(df['homerseklet'] > 0) & (df['frekvencia'] > 0)]
df = df.loc['2026-08-26':'2026-08-28'].copy()
print(f"Elemzett mérési pontok száma: {len(df)} db")

#Lineáris regresszió
korrelacio = df['homerseklet'].corr(df['frekvencia'])
print(f"Hőmérséklet-Frekvencia korreláció: {korrelacio:.3f}")

m, b = np.polyfit(df['homerseklet'], df['frekvencia'], 1)
df['alapvonal_frek'] = m * df['homerseklet'] + b

#Adatok korrekciója
df['korrigalt_frekvencia'] = df['frekvencia'] - df['alapvonal_frek']

#Csúcsok keresése
csucs_indexek, _ = find_peaks(df['korrigalt_frekvencia'], height=50, distance=3)
print(f"Detektált anomáliák száma: {len(csucs_indexek)} db")

#Vizualizáció
fig, axs = plt.subplots(3, 1, figsize=(14, 12))

# Felső grafikon: Eredeti adatok
axs[0].plot(df.index, df['frekvencia'], label='Frekvencia (Hz)', color='#f39c12', alpha=0.8)
axs[0].plot(df.index, df['alapvonal_frek'], label='Hőmérséklet-függő alapvonal', color='red', linestyle='--', linewidth=2)
axs[0].set_title('1. Eredeti frekvencia és a hőmérsékleti alapvonal', fontweight='bold')
axs[0].set_ylabel('Frekvencia (Hz)')
axs[0].legend()
axs[0].grid(True, linestyle=':', alpha=0.6)

# Középső grafikon: Hőmérséklet
axs[1].plot(df.index, df['homerseklet'], label='Hőmérséklet (°C)', color='#e83e8c', linewidth=2)
axs[1].set_title('2. Hőmérséklet alakulása', fontweight='bold')
axs[1].set_ylabel('Hőmérséklet (°C)')
axs[1].legend()
axs[1].grid(True, linestyle=':', alpha=0.6)

# Alsó grafikon: Tisztított adatok és csúcsok
axs[2].plot(df.index, df['korrigalt_frekvencia'], label='Korrigált frekvencia', color='#2980b9', alpha=0.9)
axs[2].plot(df.index[csucs_indexek], df['korrigalt_frekvencia'].iloc[csucs_indexek], "X", color='red', markersize=12, label='Riasztási csúcsok')
axs[2].axhline(y=50, color='gray', linestyle=':', label='Küszöb (50 Hz)')
axs[2].set_title('3. Anomáliák a hőmérséklet-korrigált adatokon', fontweight='bold')
axs[2].set_ylabel('Eltérés (Hz)')
axs[2].legend()
axs[2].grid(True, linestyle=':', alpha=0.6)

plt.tight_layout()
plt.show()