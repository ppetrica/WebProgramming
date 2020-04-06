class Produs {
    constructor(id, nume, cantitate) {
        this.id = id;
        this.nume = nume;
        this.cantitate = cantitate;
    }
}


function toRow(object) {
    const row = document.createElement("tr");
    
    const id_col = document.createElement("td");
    const name_col = document.createElement("td");
    const quantity_col = document.createElement("td");
    
    id_col.innerHTML = object.id;
    name_col.innerHTML = object.nume;
    quantity_col.innerHTML = object.cantitate;

    row.appendChild(id_col);
    row.appendChild(name_col);
    row.appendChild(quantity_col);

    return row;
}


function adaugaProdus() {
    const name = document.getElementById("name").value;
    const quantity = document.getElementById("quantity").value;

    let products = localStorage.getItem("products");
    if (products) {
        products = JSON.parse(products);
    } else {
        products = []
    }

    let lastId = localStorage.getItem("lastId");
    if (lastId) {
        lastId = parseInt(lastId) + 1;
    } else {
        lastId = 1;
    }

    const product = new Produs(lastId, name, quantity);
    
    products.push(product);

    localStorage.setItem("products", JSON.stringify(products));
    localStorage.setItem("lastId", lastId);

    const table = document.getElementById("product-table");
    let body = table.getElementsByTagName("tbody")[0];

    body.innerHTML = "";

    for (const product of products) {
        body.appendChild(toRow(product));
    }
}
