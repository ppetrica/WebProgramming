function showInfo() {
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
}


function extractLotoNumbers() {
    let loto_number_div = document.getElementById("loto-numbers-div");
    let loto_number_elems = loto_number_div.children;
    
    console.log(loto_number_elems);
    
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