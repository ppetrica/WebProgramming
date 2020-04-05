function createTable() {
    let request = new XMLHttpRequest();

    request.onreadystatechange = () => {
        if (request.readyState == 4 && request.status == 200) {
            const xmlDoc = request.responseXML;

            let table = document.createElement("table");

            let header = document.createElement("thead");

            let header_row = document.createElement("tr");

            const column_names = ["id", "nume", "prenume", "varsta", "adresa"];
            for (const name in column_names) {
                let col = document.createElement("th");
                col.innerHTML = name;

                header_row.appendChild(col);
            }

            header.appendChild(header_row);

            table.appendChild(header);

            let body = document.createElement("tbody");

            const persons = xmlDoc.getElementsByTagName("persoana");
            
            for (const person of persons) {
                let row = document.createElement("tr");

                const id = person.getAttribute("id");
                let c_id = document.createElement("td");
                c_id.innerHTML = id;
                row.appendChild(c_id);

                const firstName = person.getElementsByTagName("nume")[0];
                let c_firstName = document.createElement("td");
                c_firstName.innerHTML = firstName.innerHTML;
                row.appendChild(c_firstName);

                const lastName = person.getElementsByTagName("prenume")[0];
                let c_lastName = document.createElement("td");
                c_lastName.innerHTML = lastName.innerHTML;
                row.appendChild(c_lastName);

                const age = person.getElementsByTagName("varsta")[0];
                let c_Age = document.createElement("td");
                c_Age.innerHTML = age.innerHTML;
                row.appendChild(c_Age);

                const address = person.getElementsByTagName("adresa")[0];
                const street = address.getElementsByTagName("strada")[0];
                const number = address.getElementsByTagName("numar")[0];
                const city = address.getElementsByTagName("localitate")[0];
                const region = address.getElementsByTagName("judet")[0];
                const country = address.getElementsByTagName("tara")[0];

                let c_address = document.createElement("td");
                c_address.innerHTML = street.innerHTML + ' ' + number.innerHTML + ' ' + city.innerHTML + ' ' + region.innerHTML + ' ' + country.innerHTML;
                row.appendChild(c_address);
                
                body.appendChild(row);
            }

            table.appendChild(body);

            let content = document.getElementById("continut")
            
            content.innerHTML = "";

            content.appendChild(table);
        }
    }

    request.open("GET", "/resurse/persoane.xml", true);
    request.setRequestHeader("Accept", "application/xml");

    request.send();
}