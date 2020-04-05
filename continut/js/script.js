function initializePage() {
    document.getElementById("info-url").innerHTML = window.location.toString();
    document.getElementById("info-os").innerHTML = navigator.platform;
    document.getElementById("info-browser").innerHTML = navigator.appName;

    let date_displayer = () => {
        let date = new Date();

        document.getElementById("info-date-time").innerHTML = date.toString();    
    }

    setInterval(date_displayer, 1);

    let error_callback = (err) => {
        document.getElementById("info-geo-location").innerHTML =
            `ERROR(${err.code}): ${err.message}`;
    }

    let success_callback = (position) => {
        let coords = position.coords;

        document.getElementById("info-geo-location").innerHTML =
            `Latitudine: ${coords.latitude} Longitudine: ${coords.longitude}`;  
    }

    window.navigator.geolocation.getCurrentPosition(success_callback, error_callback, {});


    let click = 0;

    let last_position_x;
    let last_position_y;

    const canvas = document.getElementById("drawing-canvas");
    const color = document.getElementById("drawing-color");

    canvas.addEventListener("click", (click_event) => {
        if (click == 0) {
            last_position_x = click_event.clientX;
            last_position_y = click_event.clientY;
            
            click = 1;
        } else {
            const rect = canvas.getBoundingClientRect()
            
            let context = canvas.getContext("2d");

            context.fillStyle = color.value;
            context.fillRect(last_position_x - rect.left,
                             last_position_y - rect.top,
                             click_event.clientX - last_position_x,
                             click_event.clientY - last_position_y);
            
            click = 0;
        }
    });

    const table = document.getElementById("resizable-table");
    
    const column_number = document.getElementById("add-column-number");
    const column_color = document.getElementById("add-column-color");

    let column_button = document.getElementById("add-column-button");
    column_button.onclick = () => {
        const col = parseInt(column_number.value, 10);

        if (table.children.length == 0) {
            let new_row = document.createElement("tr");
    
            let new_cell = document.createElement("td");
            new_cell.style.backgroundColor = column_color.value;
            new_cell.innerHTML = "&nbsp;&nbsp;&nbsp;";

            new_row.appendChild(new_cell);

            table.appendChild(new_row);
    
            return;
        }
        
        for (let i = 0; i < table.children.length; ++i) {
            row = table.children[i];

            let new_cell = document.createElement("td");
            new_cell.style.backgroundColor = column_color.value;
            new_cell.innerHTML = "&nbsp;&nbsp;&nbsp;";
            
            if (col < row.children.length) {
                row.insertBefore(new_cell, table.children[col]);
            } else {
                row.appendChild(new_cell);
            }
        }
    }

    const row_number = document.getElementById("add-row-number");
    const row_color = document.getElementById("add-row-color");

    let row_button = document.getElementById("add-row-button");
    row_button.onclick = () => {
        const row = parseInt(row_number.value, 10);

        const len = (table.children.length == 0) ? 1 : table.children[0].children.length;

        let new_row = document.createElement("tr");
        for (let i = 0; i < len; ++i) {
            let new_cell = document.createElement("td");
            new_cell.innerHTML = "&nbsp;&nbsp;&nbsp;";
            new_cell.style.backgroundColor = row_color.value;

            new_row.appendChild(new_cell);
        }

        if (row < table.children.length) {
            table.insertBefore(new_row, table.children[row]);
        } else {
            table.appendChild(new_row);
        }
    };
}


function extractLotoNumbers() {
    let loto_number_div = document.getElementById("loto-numbers-div");
    let loto_number_elems = loto_number_div.children;
    
    let loto_numbers_input = document.getElementById("loto-numbers-input");
    let user_loto_numbers_elems = loto_numbers_input.children;
    
    let guessed = 0;
    for (let i = 0; i < loto_number_elems.length; ++i) {
        let number = Math.round(Math.random() * 255);
        
        let hex = number.toString(16).toUpperCase();

        if (hex.length != 2) {
            hex = '0' + hex;
        }

        let user_number = parseInt(user_loto_numbers_elems[i].value, 16);
        if (user_number == number) {
            ++guessed;
        }

        loto_number_elems[i].value = hex;
    }

    let message_div = document.getElementById("loto-response-text");

    let hue = (guessed / loto_number_elems.length * 120).toString(10);
    message_div.style.backgroundColor = "hsl(" + hue + ", 50%, 70%)";
    
    message_div.innerHTML = "Dumneavoastră ați ghicit " + guessed + ((guessed == 1) ? " număr." : " numere.");
}


function schimbaContinut(resursa, jsFisier, jsFunctie) {
    let request = new XMLHttpRequest();

    request.onreadystatechange = () => {
        if (request.readyState == 4 && request.status == 200) {
            document.getElementById("continut").innerHTML = request.responseText;
            
            if (jsFisier) {
                var elementScript = document.createElement('script');
                elementScript.onload = function () {
                    console.log("hello");
                    if (jsFunctie) {
                        window[jsFunctie]();
                    }
                };
                elementScript.src = jsFisier;
                document.head.appendChild(elementScript);
            } else {
                if (jsFunctie) {
                    window[jsFunctie]();
                }
            }
        }
    }

    request.open("GET", resursa + ".html", true)
    request.send();
}