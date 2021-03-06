# linkedboxdraw

Edition of relational diagrams, web based, serverless. 

Typical use case:

Just been hired on a new software or data development project. You need to quickly understand how the data is structured. With linkedboxdraw, you can quickly create your own map. You type in the logical information, the diagram geometry will be computed by algorithms. 

https://ludoaubert.github.io/linkedboxdraw/tableinput.html

Is a simple GUI where you can type in your box (table) names, and a list of fields (columns) that belong to each box. You can also create logical links from one box to another (foreign keys). Sometimes, a box might be connected to almost all other boxes, in which case you might want to express these logical links using field color matching instead of geometric links.
You can save your diagram data to a json file (see example provided https://github.com/ludoaubert/linkedboxdraw/blob/master/diagdata.js). This file does not contain geometric information. You can also compute and save a geometric information file (see example provided https://github.com/ludoaubert/linkedboxdraw/blob/master/contexts.js). The process to compute this geometric information is :
1) minimum cut: break the diagram down into clusters, with a maximum of 20 boxes per cluster, cutting as few links as possible.
2) box layout: compute the translation of each box in its cluster, in such a way that connected boxes should possibly lie close to each other.
3) links: compute geometric links materialized by New York City Street route like polylines (interconnected North South or East West segments). Polylines should be as short as possible, make as few turns as possible, cross each other as seldom as possible. 

This geometric information file contains cluster numbers, box translations and polylines.
Both files have a json format (although a .js extension).

https://ludoaubert.github.io/linkedboxdraw/connected_rectangles.html

Is where you can view the diagram itself. It is also possible to edit the geometric information in two ways.
1) by moving a box (click and drag), which will also trigger a recomputation of the geometric links.
2) by updating the repartition: The repartition table holds the cluster (zero based) number for each box. By updating it and pressing "Apply Repartition", you can move a box from one cluster to another.

Used Technology:

The project depends on Eigen https://eigen.tuxfamily.org/ for the minimum cut (breaking the diagram down into clusters).
It is deployed on github.io.
The Algorithm are implemented in C++ and compiled to Web Assemblies for easy deployment on the web.

Your Diagram Data Privacy:

Your diagrams may hold sensitive information.
Your diagram data will never be uploaded anywhere. It is held in two files which are yours. It is up to you to share them with whoever you want (for example via e-mail, github...), or not.
