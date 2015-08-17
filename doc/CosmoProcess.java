import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;



/**
 * process the cosmo template of docstrap for jsdoc.
 * Put this file into the parent directory of generated doc package.
 *
 * @author bean
 *
 */
public class CosmoProcess {
    private ArrayList<String> deletedConstructors=new ArrayList<String>();
    /**
     * Remove the dot in static function to enable anchor redirection. (change
     * inplace.)
     * @param journalhtml
     * @throws IOException
     */
    public void replaceAnchorForStaticClass(String journalhtml) throws IOException {
        BufferedReader htmlreader = new BufferedReader(new FileReader(journalhtml));
        /*
         * 1. read all the lines 2. process in temp lines array 3. reopen and
         * write inplace format: change <a href="#.createRoom">&lt;static&gt;
         * createRoom(name, callback, callbackError, options, params)</a>
         */
        // temp file lines
        ArrayList<String> lines = new ArrayList<String>();
        String line;
        String lastline = "";
        Pattern r = Pattern
                .compile("<h4 class=\\\"name\\\" id=\\\"(\\.[^\"]*)\\\"><span class=\\\"type-signature\\\">&lt;static");
        Matcher m;
        while ((line = htmlreader.readLine()) != null) {
            m = r.matcher(line);
            if (line.trim().equals("")) {
                // skip
            } else if (lastline.trim().equals(line.trim()) && line.contains("page-title")) {
                // skip this line
                lastline = line;
            } else {
                if (m.find()) {
                    int groupidx = line.indexOf(m.group(1));
                    line = line.substring(0, groupidx) + line.substring(groupidx + 1);
                }
                lines.add(line + "\n");
                lastline = line;
            }
        }
        htmlreader.close();

        File out = new File(journalhtml);
        FileWriter writer = new FileWriter(out);
        for (String writeline : lines) {
            writer.write(writeline);
        }
        writer.close();
    }

    public void replaceStaticAnchorInDir(String filepath) throws IOException {
        File dir = new File(filepath);
        if (dir.isDirectory()) {
            File[] files = dir.listFiles();
            for (File file : files) {
                String filename = file.getName();
                if (file.isDirectory()) {
                    // skip
                } else if (filename.contains(".js.html")) {
                    file.delete();
                    System.out.println("delete file:" + filename);
                } else {
                    this.replaceAnchorForStaticClass(file.getPath());
                }
            }
        } else {
            System.out.println("Not a directory!");
        }
        System.out.println("Fix anchor for static function: done...");
    }

    /**
     * remove scoll highlight event.
     *
     * @param filepath
     * @throws FileNotFoundException
     */
    public void closeScollHighlight(String filepath) throws IOException {
        String tocjsfile = filepath + File.separator + "scripts" + File.separator + "toc.js";
        BufferedReader reader = new BufferedReader(new FileReader(tocjsfile));
        String line;
        ArrayList<String> lines = new ArrayList<String>();
        while ((line = reader.readLine()) != null) {
            if (line.trim().equals("highlightOnScroll: true,")) {
                line = "highlightOnScroll: false,";
            }
            lines.add(line + "\n");
        }
        reader.close();
        FileWriter writer = new FileWriter(new File(tocjsfile));
        for (String writeline : lines) {
            writer.write(writeline);
        }
        writer.close();
        System.out.println("Close scroll: done...");
    }

    public void deleteDupTitle(String dirpath) throws IOException {
        String filepath = dirpath + File.separator + "index.html";
        BufferedReader reader = new BufferedReader(new FileReader(filepath));
        String line;
        ArrayList<String> lines = new ArrayList<String>();
        while ((line = reader.readLine()) != null) {
            if (!line.trim().equals("<span class=\"page-title\">Index</span>")) {
                lines.add(line + "\n");
            }
        }
        reader.close();
        FileWriter writer = new FileWriter(filepath);
        for (String writeline : lines) {
            writer.write(writeline);
        }
        writer.close();
        System.out.println("Delete duplicate title: done...");
    }

    public void changeFontWeight(String dirpath) throws IOException{
        String filepath=dirpath+File.separator+"styles"+File.separator+"site.cosmo.css";
        //read and store in temp array
        ArrayList<String> lines=new ArrayList<String>();
        BufferedReader reader=new BufferedReader(new FileReader(filepath));
        String line;
        String lastline="";
        while((line=reader.readLine())!=null){
            if(line.trim().equals("font-weight: 300;")&&lastline.trim().startsWith("font-family")){
                System.out.println(line+"\n"+lastline);
                line="/* font-weight: 300; */";
            }
            if(line.trim().equals(".navbar-brand {")){
                line=".navbar-brand { \n"
                        //+ "margin-right: 100px;\n"
                        + "margin-top:10px;\n";
                String templine;
                while(!(templine=reader.readLine()).trim().equals("}")){
                    if(line.contains("font-size")){
                        line+="font-size: 23px;\n";
                        continue;
                    }
                    line+=templine+"\n";
                }
                line+="}";
            }
            if(line.trim().equals(".container {")){
                String templine;
                while(!(templine=reader.readLine()).trim().equals("}")){
                    if(line.contains("margin-left")){
                        line+="margin-left: 10px;\n";
                        continue;
                    }
                    line+=templine+"\n";
                }
                line+="}";
            }
            if(line.trim().equals(".navbar-header {")){
                line+="\nheight:60px;";
            }
            if(line.trim().equals(".navbar-inverse {")){
                String templine;
                while(!(templine=reader.readLine()).trim().equals("}")){
                    if(line.contains("background-color")){
                        line+="background-color: #0071c5;\n";
                        continue;
                    }
                    line+=templine+"\n";
                }
                line+="}";
            }
            lines.add(line+"\n");
            lastline=line;
        }
        FileWriter writer=new FileWriter(new File(filepath));
        for(String writeline:lines){
            writer.write(writeline);
        }
        reader.close();
        writer.close();
        System.out.println("Comment font-size: done...");
    }

    private void deleteConstructor(String filepath) throws IOException{
        /*
         * <div class="container-overview"> ... </div>
         * if there're more than 9 lines, write out constructor, else delete the span
         */
        BufferedReader reader=new BufferedReader(new FileReader(filepath));
        //read and store the lines
        ArrayList<String> lines=new ArrayList<String>();
        ArrayList<String> divlines=new ArrayList<String>();
        String line;
        boolean storeinlines=true;
        Pattern p=Pattern.compile("<h4 class=\"name\" id=\"([^>]*)\">");
        Matcher m;
        String deletecons="";
        while((line=reader.readLine())!=null){
            //process flag
            if(line.trim().equals("<div class=\"container-overview\">")){
                storeinlines=false;
            }else if(line.trim().equals("</div>")&&!storeinlines){
                divlines.add(line);
                if(divlines.size()>9){
                    for(String writeline:divlines){
                        lines.add(writeline);
                    }
                }else if(!deletecons.equals("")){
                    deletedConstructors.add(deletecons);
                }
                storeinlines=true;
                continue;
            }else if(!storeinlines){
                m=p.matcher(line);
                if(m.find()){
                    deletecons=m.group(1);
                }
            }
            if(storeinlines){
                lines.add(line+"\n");
            }else{
                divlines.add(line+"\n");
            }
        }
        reader.close();
        //rewrite
        FileWriter writer=new FileWriter(filepath);
        for(String writeline:lines){
            writer.write(writeline);
        }
        writer.close();
    }
    public void deleteConstrutorHelper(String dirpath) throws IOException{
        File dir = new File(dirpath);
        if (dir.isDirectory()) {
            File[] files = dir.listFiles();
            for (File file : files) {
                String filename = file.getName();
                if (file.isDirectory()) {
                    // skip
                } else if (filename.contains(".js.html")) {
                    file.delete();
                    System.out.println("delete file:" + filename);
                } else {
                    //process *.html files
                    this.deleteConstructor(file.getPath());
                    this.addLogoInHTML(file.getPath());
                    this.modifyHeaderStyle(file.getPath());
                    this.addHomeInNav(file.getPath());
                }
            }
        } else {
            System.out.println("Not a directory!");
        }
        System.out.println("Delete constructors: ");
        for(String cons:deletedConstructors){
            System.out.println(cons);
        }
        System.out.println("done...");
    }
    public void removeCodeHighlightLogo(String dirpath) throws IOException{
        String filepath=dirpath+File.separator+"styles"+File.separator+"sunlight.default.css";
        BufferedReader reader=new BufferedReader(new FileReader(filepath));
        ArrayList<String> lines=new ArrayList<String>();
        String line;
        while((line=reader.readLine())!=null){
            if(line.trim().equals(".sunlight-menu {")){
                lines.add(line+"\n");
                lines.add("display:none;\n");
                continue;
            }
            lines.add(line);
        }
        reader.close();
        FileWriter writer=new FileWriter(new File(filepath));
        for(String writeline:lines){
            writer.write(writeline);
        }
        writer.close();
        System.out.println("Remove code highlight logo: done...");
    }
    public void changeLinkForCreateFunction(String dirpath) throws IOException{
        /*
         * this method change the link in html of static function
         * only for Create
         */
        String filepath=dirpath+File.separator+"N.API.html";
        BufferedReader reader=new BufferedReader(new FileReader(filepath));
        String targetline="N.API.html#.createRoom";
        String line;
        ArrayList<String> writetemp=new ArrayList<String>();
        while((line=reader.readLine())!=null){
            if(line.contains(targetline)){
                System.out.println(line);
                writetemp.add("<td class=\"description last\"><p>Room configuration. See details "
                        + "about options in <a href=\"N.API.html#createRoom\">"
                        + "createRoom(name, callback, callbackError, options)</a>.</p></td>\n");
                continue;
            }
            writetemp.add(line+"\n");
        }
        reader.close();
        FileWriter writer=new FileWriter(new File(filepath));
        System.out.println(writetemp.size());
        for(String writeline:writetemp){
            writer.write(writeline);
        }
        writer.close();
        System.out.println("Rewrite N.API.html#.createRoom done...");
    }
    private void addLogoInHTML(String filepath) throws IOException{
        BufferedReader reader=new BufferedReader(new FileReader(filepath));
        ArrayList<String> writetemp=new ArrayList<String>();
        String line;
        while((line=reader.readLine())!=null){
            if(line.trim().equals("<div class=\"navbar-header\">")){
                line+="\n<img src=\"intellogo.png\" alt=\"logo\" style=\"float: left;\">";
            }
            writetemp.add(line+"\n");
        }
        reader.close();
        FileWriter writer=new FileWriter(new File(filepath));
        for(String writeline:writetemp){
            writer.write(writeline);
        }
        writer.close();
    }
    private void modifyHeaderStyle(String filepath) throws IOException{
        String cosmoCSSFile=filepath;
        BufferedReader reader=new BufferedReader(new FileReader(cosmoCSSFile));
        ArrayList<String> writetemp=new ArrayList<String>();
        String line;
        while((line=reader.readLine())!=null){
            if(line.trim().equals("<div class=\"navbar navbar-default navbar-fixed-top navbar-inverse\">")){
                line+="\n<div class=\"head_logo\">";
                //skip next line
                reader.readLine();
            }
            if(line.trim().equals("<img src=\"intellogo.png\" alt=\"logo\" style=\"float: left;\">")){
                line="<img src=\"intellogo.png\" alt=\"logo\" >";
            }
            if(line.contains("Intel®")){
                line=line.replace("Intel®", "Intel<sup>®</sup>");
            }
            if(line.trim().equals("<link type=\"text/css\" rel=\"stylesheet\" href=\"styles/site.cosmo.css\">")){
                line+="\n<link rel=\"stylesheet\" type=\"text/css\" href=\"styles/head.css\">";
            }
            if(line.trim().equals("<div class=\"navbar-header\">")){
                line="<div class=\"navbar_header\">";
            }
            if(line.trim().equals("<ul class=\"nav navbar-nav\">")){
                line="<ul class=\"nav navbar_nav\">";
            }
            if(line.trim().startsWith("<a class=\"navbar-brand\" href=")){
                String tmp="<a class=\"navbar-brand\" href=";
                line="<a class=\"navbar_brand\" href="+line.substring(line.indexOf(tmp)+tmp.length());
                System.out.println("navbrand after replace:"+line);
            }
            if(line.trim().contains("dropdown-toggle")){
                line=line.replace("dropdown-toggle", "dropdown_toggle");
                System.out.println("dropdown_toggle after replace:"+line);
            }
            writetemp.add(line+"\n");
        }
        reader.close();
        FileWriter writer=new FileWriter(new File(filepath));
        for(String writeline:writetemp){
            writer.write(writeline);
        }
        writer.close();
    }
    public void commentAnimateCallback(String dirpath) throws IOException{
        String tocFilepath=dirpath+File.separator+"scripts"+File.separator+"toc.js";
        BufferedReader reader=new BufferedReader(new FileReader(tocFilepath));
        ArrayList<String> writetemp=new ArrayList<String>();
        String line;
        while((line=reader.readLine())!=null){
            if(line.trim().equals("location.hash = elScrollTo;")){
                line="//location.hash = elScrollTo;";
            }
            writetemp.add(line+"\n");
        }
        reader.close();
        FileWriter writer=new FileWriter(new File(tocFilepath));
        for(String writeline:writetemp){
            writer.write(writeline);
        }
        writer.close();
    }
    private void addHomeInNav(String filepath) throws IOException{
        BufferedReader reader=new BufferedReader(new FileReader(filepath));
        ArrayList<String> writetemp=new ArrayList<String>();
        String line;
        System.out.println("addHomeInNav");
        while((line=reader.readLine())!=null){
            if(line.trim().equals("<ul class=\"nav navbar_nav\">")){
                System.out.println("find navbar_nav");
                line+="<li class=\"dropdown\">\n<a href=\"index.html\">Home</a>\n</li>";
                System.out.println("line after modify:"+line);
            }
            if(line.trim().startsWith("<title>")){
                line="<title>Intel CS for WebRTC Client SDK for JavaScript</title>";
            }
            writetemp.add(line+"\n");
        }
        reader.close();
        FileWriter writer=new FileWriter(new File(filepath));
        for(String writeline:writetemp){
            writer.write(writeline);
        }
        writer.close();
    }
    public static void main(String args[]) throws IOException {
        CosmoProcess test = new CosmoProcess();
        test.replaceStaticAnchorInDir(args[0]);
        test.deleteDupTitle(args[0]);
        test.closeScollHighlight(args[0]);
        test.changeFontWeight(args[0]);
        test.deleteConstrutorHelper(args[0]);
        test.removeCodeHighlightLogo(args[0]);
        test.changeLinkForCreateFunction(args[0]);
        test.commentAnimateCallback(args[0]);
    }
}
