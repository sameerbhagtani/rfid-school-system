const app = document.getElementById('app');

const API_BASE_URL = `http://${window.location.hostname}:3000/api`;

const students = [
    { name: 'Sameer', id: '53CC0829' },
    { name: 'Abhishek', id: '6286FA05' },
    { name: 'Monish', id: '23669213' }
];

// --- Views ---

function renderHome() {
    const studentListHtml = students.map(student => `
        <a href="/analytics/${student.id}" class="student-link" data-link>
            ${student.name} <span style="float:right; opacity:0.6; font-family:monospace">${student.id}</span>
        </a>
    `).join('');

    return `
        <div class="container">
            <h1>RFID School System</h1>
            <div class="student-list">
                ${studentListHtml}
            </div>
        </div>
    `;
}

async function renderAnalytics(id) {
    // Initial loading state
    app.innerHTML = `
        <div class="container analytics-card">
            <h1>Analytics</h1>
            <p class="loading">Loading data for ID: ${id}...</p>
        </div>
    `;

    try {
        const response = await fetch(`${API_BASE_URL}/get-analytics/${id}`);

        if (!response.ok) {
            throw new Error(`Server returned ${response.status} ${response.statusText}`);
        }

        const result = await response.json();

        if (result.success && result.data) {
            const data = result.data;
            const performanceClass = data.performance === 'above' ? 'perf-above' :
                data.performance === 'below' ? 'perf-below' : 'perf-average';

            const calendarHtml = renderCalendar(data.attendanceDates || [], data.holidayDates || []);

            app.innerHTML = `
                <div class="container analytics-card">
                    <div class="analytics-header">
                        <h2>Analytics: ${data.name}</h2>
                        <button class="back-btn" onclick="navigateTo('/')">Back to Home</button>
                    </div>
                    
                    ${calendarHtml}

                    <div class="stat-grid">
                        <div class="stat-item">
                            <span class="stat-label">Streak</span>
                            <div class="stat-value">${data.streak} Days</div>
                        </div>
                        <div class="stat-item">
                            <span class="stat-label">Performance</span>
                            <div class="performance-badge ${performanceClass}">${data.performance.toUpperCase()}</div>
                        </div>
                        <div class="stat-item">
                            <span class="stat-label">Student %</span>
                            <div class="stat-value">${data.studentPercent}%</div>
                        </div>
                        <div class="stat-item">
                            <span class="stat-label">Class Avg %</span>
                            <div class="stat-value">${data.classAvgPercent}%</div>
                        </div>
                    </div>
                </div>
            `;
        } else {
            // Success false or no data
            renderError("Some error occurred");
        }
    } catch (error) {
        console.error("Fetch error:", error);
        renderError(`Some error occurred: ${error.message}`);
    }
}

function renderCalendar(attendanceDates, holidayDates) {
    const today = new Date();
    const currentMonth = today.getMonth();
    const currentYear = today.getFullYear();

    // Get first day of the month
    const firstDay = new Date(currentYear, currentMonth, 1);
    const lastDay = new Date(currentYear, currentMonth + 1, 0);

    // Days in current month
    const daysInMonth = lastDay.getDate();
    // Day of week the month starts on (0=Sunday)
    const startDayOfWeek = firstDay.getDay();

    const monthNames = ["January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    ];

    let html = `
        <div class="calendar-wrapper">
            <h3 class="calendar-title">${monthNames[currentMonth]} ${currentYear}</h3>
            <div class="calendar-grid">
                <div class="calendar-header">Sun</div>
                <div class="calendar-header">Mon</div>
                <div class="calendar-header">Tue</div>
                <div class="calendar-header">Wed</div>
                <div class="calendar-header">Thu</div>
                <div class="calendar-header">Fri</div>
                <div class="calendar-header">Sat</div>
    `;

    // Empty cells for days before start of month
    for (let i = 0; i < startDayOfWeek; i++) {
        html += `<div class="calendar-day empty"></div>`;
    }

    // Days of the month
    for (let day = 1; day <= daysInMonth; day++) {
        const dateStr = `${currentYear}-${String(currentMonth + 1).padStart(2, '0')}-${String(day).padStart(2, '0')}`;
        // Note: dateStr needs to match API format "YYYY-MM-DD"

        let statusClass = '';
        const isToday = day === today.getDate();
        const fullDate = new Date(currentYear, currentMonth, day);

        if (attendanceDates.includes(dateStr)) {
            statusClass = 'status-present';
        } else if (holidayDates.includes(dateStr)) {
            statusClass = 'status-holiday';
        } else if (fullDate < today) { // Mark absent only for past days
            statusClass = 'status-absent';
        }

        html += `
            <div class="calendar-day ${statusClass} ${isToday ? 'current-day' : ''}">
                <span class="day-number">${day}</span>
            </div>
        `;
    }

    html += `
            </div>
            <div class="calendar-legend">
                <div class="legend-item"><span class="dot status-present"></span> Present</div>
                <div class="legend-item"><span class="dot status-absent"></span> Absent</div>
                <div class="legend-item"><span class="dot status-holiday"></span> Holiday</div>
            </div>
        </div>
    `;

    return html;
}

function renderError(message) {
    app.innerHTML = `
        <div class="container analytics-card">
             <div class="analytics-header">
                <h2>Error</h2>
                <button class="back-btn" onclick="navigateTo('/')">Back to Home</button>
            </div>
            <div class="error-message">
                ${message}
            </div>
        </div>
    `;
}

// --- Router ---

function navigateTo(url) {
    history.pushState(null, null, url);
    router();
}

async function router() {
    const path = window.location.pathname;

    // Check for Analytics route: /analytics/:id
    const analyticsMatch = path.match(/^\/analytics\/([a-zA-Z0-9]+)$/);

    if (path === '/' || path === '/index.html') {
        app.innerHTML = renderHome();
    } else if (analyticsMatch) {
        const id = analyticsMatch[1];
        await renderAnalytics(id);
    } else {
        // Simple 404 for unknown routes, redirect to home or show error
        app.innerHTML = `
            <div class="container">
                <h1>404 - Page Not Found</h1>
                <button class="back-btn" onclick="navigateTo('/')">Go Home</button>
            </div>
        `;
    }
}

// Intercept link clicks for SPA navigation
document.addEventListener('DOMContentLoaded', () => {
    document.body.addEventListener('click', e => {
        if (e.target.closest('[data-link]')) {
            e.preventDefault();
            navigateTo(e.target.closest('[data-link]').href);
        }
    });

    // Handle back/forward buttons
    window.addEventListener('popstate', router);

    // Initial render
    router();
});
