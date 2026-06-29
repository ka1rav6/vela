import { processFile } from "./lexer.js"
import { Parser      } from "./parser.js" 

async function main(){
    const argvs   = process.argv;
    if (argvs.length < 2){
        throw new Error("Did not recieve any file as input");
    }
    const inpFile   = argvs[2];                         // second argument is the input file name/ path
    const tokenList = processFile(inpFile);             // tokens[][] returned from the process file function : Lexer work done.
    const parser    = new Parser(); 
    const program   = parser.parseTokenLines(tokenList);// returns an object wih type : "program" and statements[] array
}
