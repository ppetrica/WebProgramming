function showInfo() {
    document.getElementById("info-url").innerHTML = window.location.toString();

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
