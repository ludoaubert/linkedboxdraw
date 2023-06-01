# linkedboxdraw

# Edition of relational diagrams, web based, serverless. 

Typical use case:

Just been hired on a software (or data) project. You need to quickly understand how the data is structured. With linkedboxdraw, you can quickly create your own map.
To look at an example of map : https://ludoaubert.github.io/linkedboxdraw/table_edit_ti.html

In the case of relational diagrams, there are some SQL scripts that are available to produce a diagram file.

SQL Server : https://ludoaubert.github.io/linkedboxdraw/SelectMetaData_SQLServer.sql

MySQL : https://ludoaubert.github.io/linkedboxdraw/SelectMetaData_MySQL.sql

other engines : you can create your own script and share it.

You can then proceed to load the diagram file by pressing the "File Choose" button in the "File Input" section of https://ludoaubert.github.io/linkedboxdraw/table_edit_ti.html

Or you can enter the information (box titles, box fields and links) manually.

Section "Table Edit" is a simple GUI where you can type in your box (table) names, and a list of fields (columns) that belong to each box. You can also create logical links from one box to another (foreign keys) by using the "Box Link" section. Sometimes, a box might be connected to almost all other boxes, in which case you might want to express these logical links using field color matching instead of geometric links. In this case , you might use the "Field Color" section. A field can also be an image.

You can save your diagram data to a json file by pressing "Save As" in the "File Output" section.

https://ludoaubert.github.io/linkedboxdraw/table_edit_ti.html

Is where you can view the diagram. It is also possible to edit the geometric information in two ways.
1) by moving a box (click and drag), which will also trigger a recomputation of the geometric links.
2) by updating the repartition: The repartition table holds the cluster (zero based) number for each box. By updating it and pressing "Apply Repartition", you can move a box from one cluster to another.

Cut links: geometric links do not cross cluster boundaries. Links cut by the clustering algorithms are materialized using field color matching. This information is computed on the fly and is not persisted.

Used Technology:

The project depends on Eigen https://eigen.tuxfamily.org/ for the clustering (minimum cut) algorithm.
It is deployed on github.io.
The Algorithms are implemented in C++ and compiled to Web Assemblies for easy deployment on the web.
