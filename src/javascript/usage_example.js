FILE_WASM='downward.wasm'
FILE_JAVASCRIPT='downward.js'
PATH_TO_INPUT='output.sas'

// Load the javascript file containing the Downward module
downwardscript = document.createElement('script');
downwardscript.src = FILE_JAVASCRIPT;
downwardscript.onload = function() {
    // Fetch input data via XHR and save it on the browsers virtual filesystem once it is loaded
    console.log("Fetching data from " + PATH_TO_INPUT);
    var inputURL = PATH_TO_INPUT;
    var inputXHR = new XMLHttpRequest();
    inputXHR.open('GET', inputURL, true);
    inputXHR.responseType = 'text';
    inputXHR.onload = function() {
        if (inputXHR.status === 200 || inputXHR.status === 0) {
            let data = new TextEncoder().encode(inputXHR.response);
            let stream = FS.open('output.sas', 'w+');
            FS.write(stream, data, 0, data.length, 0);
            FS.close(stream);
            console.log('wrote to output.sas');
        }
    }
    inputXHR.send();
}
document.body.appendChild(downwardscript);

// function to start the actual program
function runFastDownward() {
// define parameters and split them to a list
    let parameter_string = "--search astar(lmcut()) --input output.sas"
	let parameter_list = parameter_string.split(" ")

    Module.callMain(parameter_list);

// results are saved on the virtual filesystem in the browser
    let result = new TextDecoder().decode(FS.readFile('sas_plan'));
    console.log(result);
}
