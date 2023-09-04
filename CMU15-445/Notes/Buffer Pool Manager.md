# Buffer Pool Manager

- [Buffer Pool Manager](#buffer-pool-manager)
  - [Introduction](#introduction)
    - [Basic Concept](#basic-concept)
    - [Relational Model](#relational-model)
    - [Summary](#summary)


## Introduction

### Basic Concept

*Database* is a organized collection of inter-related data that modles the aspect of the real world, and the *database management system*(DBMS) is a software to management database. Therefore, database is a set of data, and DBMS can be thought of a interface to control the database.

Database is stored as comma-seperqted value(CSV) file that DBMS manages, every entity well be stored in its own file, and application has to prase the CSV file to read or write recoed. Each entity has its own set of attributes, so **defferent record** are delimited by new line, while each of corresponding attributes **within a record** are delimited by comma

From this perspective, database can be thought of as a collection of multiple files

Consider a database models a digital music store, the two basic entities aes artist and album, so we need two CSV file to store every record in entities

*Data model* is a collection of concept for describing the data in database.

*Schema* is a description of a particular collection of data based on *data model*.

### Relational Model

Early database application were diffcult to build and maintain because there was a tight copluing between logical layers and physical layers, therefore the *relational model* was gradually proposed.

*Relational model* define database abstraction **based on relations** to avoid maintainance overhead.

It has three key points: 

* Store database in simple structure(relations)
* Access data through high-level language, DBMS just figure out best execution strategy
* Physical store(how to store data in physical layers) left up to DBMS implementation

Relation data model defines three concepts:

* Structure: The definition of *relations* and their content., what attributes *relations* contain and what value attributes can hold
* Integrity: Ensure the database's contents satisfy constrains(for example the value of attribute has to number or string)
* Manipulation: How to access and modify the database's content

*Relation* is an unordered set that contain the relationship of attributes that represent *entities*. Therefore, *relation* simplely a unordered set, and it contain a series of attrbutes, all of which have corresponding value.

*Tuple* is a set of attributes values in *relation*, value can be atomic(i.e. number, string, bool), scalar, lists and other nested data structure.

A *relations* has two *key*, which is *primary key* which **uniquely** identifies a single *tuple* and *foreign key* which specifies an attribute from one *relation* has map to a *tuple* in another *relation*

### Summary

At this point, we can draw a conclusion that: 

* *Relation* has a lot of pairs which contain *key* and *tuple*
* *tuple* simplely a set of value, each of which corresponding to single attribute

If we abstract *relation* into a table, the *attribute* is the meaning of column, and the *tuple* is the value of attribute/element in the row

