var domain;
var problem;
var pyodide;

let loadFile = async (path) => {
  return new Promise(resolve => {
    console.log(`requesting ${path}`)
    let request = new XMLHttpRequest()
    request.open('GET', path)
    request.responseType = 'text';
    request.onload = () => {
        result = request.responseText;
        console.log(result);
        resolve(result);
    }
    request.send();
  })   
}

let pythonCode_InstallTranslator = `
import micropip
micropip.install("translator-1.0-py3-none-any.whl")
`
let pythonCode_storePDDLToFilesystem = `
import js
problem = js.window.problem
domain = js.window.domain

with open('domain.pddl', 'w') as file:
    file.write(domain)
    
with open('problem.pddl', 'w') as file:
    file.write(problem)
    
`
let pythonCode_runTranslator = `
from translator.translate import run
run(["domain.pddl", "problem.pddl", "--sas-file", "output.sas"])

with open('output.sas', 'r') as file:
    output_sas = file.read()
`

let showSourceFiles = (domain, problem) => {
  domainHolder = document.querySelector("div[class='domain']").children[1];
  problemHolder = document.querySelector("div[class='problem']").children[1];
  domainHolder.innerText = domain;
  problemHolder.innerText = problem;
}

let main = async () => {
  let installTranslator = async () => {
    return pyodide.runPython(pythonCode_InstallTranslator)
  }
  let storePDDLToFilesystem = async () => {
    return pyodide.runPython(pythonCode_storePDDLToFilesystem)
  }
  let runTranslator = async () => {
    return pyodide.runPython(pythonCode_runTranslator)
  }

  // load pyodide
  pyodide = await loadPyodide({ indexURL : "https://cdn.jsdelivr.net/pyodide/v0.18.1/full/" });

  // load micropip
  await pyodide.loadPackage('micropip');

  // load pddl files
  domain = await loadFile('exampleFiles/domain.pddl');
  problem = await loadFile('exampleFiles/problem.pddl');
  showSourceFiles(domain, problem);

  // run python
  await installTranslator();
  await storePDDLToFilesystem();
  runTranslator();
  
  // read result
  let r = pyodide.globals.output_sas;
  console.log(r);
  resultHolder = document.querySelector("div[class='result']").children[0];
  resultHolder.innerText = r;
};

main();
