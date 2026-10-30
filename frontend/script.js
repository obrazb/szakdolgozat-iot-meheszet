let tempChartInstance = null;
let freqChartInstance = null;
let smsEnabled = true;

function switchTab(tabId, element) {
    document.querySelectorAll('.menu-item').forEach(el => el.classList.remove('active'));
    element.classList.add('active');

    document.getElementById('view-hives').style.display = 'none';
    document.getElementById('view-graphs').style.display = 'none';
    
    document.getElementById('view-' + tabId).style.display = 'block';

    if (tabId === 'graphs') {
        loadGraphs();
    }
}

function toggleMobileMenu() {
    const sidebar = document.getElementById('sidebar');
    sidebar.style.transform = sidebar.style.transform === 'translateX(0px)' ? 'translateX(-100%)' : 'translateX(0px)';
}

function adatokLekeres() {
    fetch('api.php?action=get_data')
        .then(response => response.json())
        .then(data => {
            const container = document.getElementById('kaptar-container');
            const graphSelect = document.getElementById('graphHiveSelect');
            container.innerHTML = ''; 
            
            if(!data || data.length === 0) {
                container.innerHTML = '<div class="col-12 text-center text-muted">Még nincsenek adatok a rendszerben.</div>';
                return;
            }

            let optionsHtml = '';

            data.forEach(kaptar => {
                optionsHtml += `<option value="${kaptar.kaptar_id}">Kaptár #${String(kaptar.kaptar_id).padStart(2, '0')}</option>`;
                
                const kijaroAllapot = parseInt(kaptar.kijaro); 
                const ujAllapot = kijaroAllapot === 0 ? 1 : 0; 
                
                const gateClass = kijaroAllapot === 0 ? 'gate-closed' : 'gate-open';
                const gateText = kijaroAllapot === 0 ? '○ Zárva' : '● Nyitva';

                let frek = parseFloat(kaptar.frekvencia);
                let actClass = 'activity-low';
                let actText = 'Alacsony';
                let actDot = '🟢';
                
                if (frek > 150 && frek <= 300) { 
                    actClass = 'activity-med'; 
                    actText = 'Közepes'; 
                    actDot = '🟡'; 
                }
                else if (frek > 300) { 
                    actClass = 'activity-high'; 
                    actText = 'Magas'; 
                    actDot = '🔴'; 
                }

                const utolsoFrissites = new Date(kaptar.idobelyeg.replace(' ', 'T')).getTime();
                const most = new Date().getTime();
                const kulonbsegPercben = (most - utolsoFrissites) / (1000 * 60);

                let statuszSzoveg = "Online";
                let statuszSzin = "#28a745"; 

                if (kulonbsegPercben > 20) {
                    statuszSzoveg = "Offline";
                    statuszSzin = "#dc3545"; 
                }

                const cardHtml = `
                    <div class="col">
                        <div class="kaptar-card h-100">
                            <div class="card-header-custom">
                                <span>Kaptár #${String(kaptar.kaptar_id).padStart(2, '0')}</span>
                                <span><span class="status-dot" style="background-color: ${statuszSzin}"></span>${statuszSzoveg}</span>
                            </div>
                            
                            <div class="temp-display">
                                <i class="fa-solid fa-temperature-half"></i>
                                <span class="temp-value">${parseFloat(kaptar.homerseklet).toFixed(1)}°C</span>
                                <span class="temp-label">Hőmérséklet</span>
                            </div>

                            <div class="activity-bar ${actClass}">
                                <span>Aktivitás</span>
                                <span>${actDot} ${actText}</span>
                            </div>

                            <div class="gate-control">
                                <span class="gate-label">Kijáró Állapota</span>
                                <button class="gate-btn ${gateClass}" data-kid="${kaptar.kaptar_id}" onclick="allapotModositas(${kaptar.kaptar_id}, ${ujAllapot})">
                                    ${gateText}
                                </button>
                            </div>

                            <div class="card-footer-custom">
                                <span><i class="fa-solid fa-wave-square"></i> ${frek.toFixed(0)} Hz</span>
                                <span class="small text-muted">Frissítve: ${kaptar.idobelyeg.substring(11,16)}</span>
                            </div>
                        </div>
                    </div>
                `;
                container.innerHTML += cardHtml;
            });

            if(graphSelect && graphSelect.options.length === 0) {
                graphSelect.innerHTML = optionsHtml;
            }
        })
        .catch(err => console.error(err));
}

function allapotModositas(kid, newState) {
    fetch(`api.php?action=set_gate&kid=${kid}&state=${newState}`)
        .then(() => adatokLekeres())
        .catch(err => console.error(err));
}

function allGates(state) {
    let uzenet = state === 1 ? "Biztosan KINYITOD az összes kaptár kijáróját?" : "Biztosan BEZÁROD az összes kaptár kijáróját?";
    
    if(confirm(uzenet)) {
        const gombok = document.querySelectorAll('.gate-btn');
        gombok.forEach(btn => {
            const kid = btn.getAttribute('data-kid'); 
            if(kid) {
                fetch(`api.php?action=set_gate&kid=${kid}&state=${state}`);
            }
        });
        
        setTimeout(adatokLekeres, 1000);
    }
}

function loadGraphs() {
    const kid = document.getElementById('graphHiveSelect').value;
    const hours = document.getElementById('graphTimeSelect').value;
    
    if (!kid) return;
    
    fetch(`api.php?action=get_history&kid=${kid}&hours=${hours}`)
        .then(response => {
            if(!response.ok) throw new Error("Hálózati hiba");
            return response.json();
        })
        .then(data => {
            if(!data || data.length === 0) return; 

            const labels = data.map(row => {
                let honapNap = row.idobelyeg.substring(5, 10);
                let oraPerc = row.idobelyeg.substring(11, 16);  
                return honapNap + " " + oraPerc;
            });
            const temps = data.map(row => parseFloat(row.homerseklet));
            const freqs = data.map(row => parseFloat(row.frekvencia));

            const ctxTemp = document.getElementById('tempChart').getContext('2d');
            if (tempChartInstance) tempChartInstance.destroy(); 
            tempChartInstance = new Chart(ctxTemp, {
                type: 'line',
                data: {
                    labels: labels,
                    datasets: [{
                        label: 'Hőmérséklet (°C)',
                        data: temps,
                        borderColor: '#e83e8c',
                        backgroundColor: 'rgba(232, 62, 140, 0.1)',
                        borderWidth: 2,
                        tension: 0.4,
                        fill: true
                    }]
                },
                options: { responsive: true, scales: { y: { suggestedMin: 20, suggestedMax: 45 } } }
            });

            const ctxFreq = document.getElementById('freqChart').getContext('2d');
            if (freqChartInstance) freqChartInstance.destroy();
            freqChartInstance = new Chart(ctxFreq, {
                type: 'line',
                data: {
                    labels: labels,
                    datasets: [{
                        label: 'Frekvencia (Hz)',
                        data: freqs,
                        borderColor: '#f39c12',
                        backgroundColor: 'rgba(243, 156, 18, 0.1)',
                        borderWidth: 2,
                        tension: 0.4,
                        fill: true
                    }]
                },
                options: { responsive: true, scales: { y: { suggestedMin: 0 } } }
            });
        })
        .catch(err => {
            console.error(err);
        });
}

function loadSMSStatus() {
    fetch('api.php?action=get_sms_status')
        .then(r => r.text())
        .then(d => { 
            smsEnabled = (d.trim() === "1"); 
            updateSMSButtonUI(); 
        });
}

function toggleSMS() {
    smsEnabled = !smsEnabled; 
    fetch(`api.php?action=set_sms_status&state=${smsEnabled ? 1 : 0}`)
        .then(() => updateSMSButtonUI());
}

function updateSMSButtonUI() {
    const btn = document.getElementById('smsToggleBtn');
    if (!btn) return;
    
    if (smsEnabled) { 
        btn.className = "btn btn-sm btn-success shadow-sm"; 
        btn.innerHTML = '<i class="fa-solid fa-bell"></i> SMS Riasztás: Aktív'; 
    } else { 
        btn.className = "btn btn-sm btn-danger shadow-sm"; 
        btn.innerHTML = '<i class="fa-solid fa-bell-slash"></i> SMS Riasztás: Kiszűrve'; 
    }
}

document.addEventListener("DOMContentLoaded", () => {
    adatokLekeres();
    loadSMSStatus();
    setInterval(adatokLekeres, 30000); 
});