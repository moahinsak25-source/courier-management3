# Courier and Parcel Management System

A menu-driven **C++ console application** for managing customers, parcel bookings, deliveries and payments. Data is stored permanently in plain text files, so no database is needed.

> MIS Console Application Assignment, Topic 14: Courier and Parcel Management System.

## Run it online (no install)

Open this link to import the project into Replit, then press **Run** (or use the Shell steps below):

https://replit.com/github.com/moahinsak25-source/courier-management1

If **Run** does nothing, open the **Shell** tab in Replit and enter:

```bash
g++ -std=c++11 -o courier courier.cpp
./courier
```

## Build and run locally

Requires any C++11 compiler (g++ or clang++).

```bash
g++ -std=c++11 -Wall -Wextra -o courier courier.cpp
./courier            # on Windows (MinGW): courier.exe
```

The program creates its data files in the folder it is run from on first start.

## Staff login

Updating a parcel's status and recording a payment are staff-only. The staff password is `admin123` (three attempts allowed).

## Features

| Menu | What it does |
|------|--------------|
| **Customer Management** | Add, display, search (ID / name / phone), update and delete customers |
| **Parcel Management** | Book parcels with automatic charge calculation, display, track, update, delete, and a charge estimator |
| **Delivery Management** | Status updates, payment recording, per-parcel history, all delivery history, pending deliveries, payments list |
| **Search & Tracking** | Track by tracking number, search by customer ID / name / phone / destination, filter by status, sort by tracking number, weight or charge |
| **Reports** | Business overview, status summary, revenue by destination, delivery history, customer activity |
| **File / Data Management** | Save now, reload, create backups, file status and integrity check |

## Charge calculation

`charge = zone base fee + (billable kg x zone rate) + extras`

| Zone | Base fee | Rate per kg |
|------|---------:|------------:|
| Local | 100 | 20 |
| Regional | 200 | 40 |
| National | 350 | 60 |
| International | 1200 | 250 |

- Weight is rounded **up** to the nearest 0.5 kg. Allowed range is 0.1 to 100 kg.
- Extras: express is **+25%** of the subtotal, insurance is **+50**, fragile handling is **+75**.

Example: a 2.3 kg Regional parcel with express and fragile handling costs 200 + (2.5 x 40) + 75 + 75 = **450.00**.

## Parcel status workflow

```
Booked -> In Transit -> Out for Delivery -> Delivered
   \           \
    +-----------+--> Cancelled   (only before Out for Delivery)
```

- An Out for Delivery parcel can also go back to In Transit.
- Delivered and Cancelled are final.
- Every status change is written to the delivery history with date, location, remarks and receiver name.
- Cancelling a paid parcel marks its payment as **Refunded**.
- If a parcel is delivered with payment pending, staff can collect cash on delivery.

## Data files

| File | Contents |
|------|----------|
| `customers.txt` | Customer records |
| `parcels.txt` | Parcel records |
| `deliveries.txt` | One line per status change (delivery history) |
| `payments.txt` | Payment records |

Fields are separated by `|`. Saves go through a temporary file first, so a failed write cannot wipe existing data. Corrupted lines are skipped with a warning when the files are loaded. Backups are written as `backup_<name>.txt`.

## Program structure

- `Record` is an abstract base class (save, load, display, key) inherited by `Customer`, `Parcel`, `Delivery` and `Payment`.
- `ChargeCalculator` holds the pricing rules.
- `CourierSystem` is the controller that owns all data and every menu.
- The `Status`, `PaymentStatus` and `InputHelper` namespaces hold the workflow rules, validation and safe console input.

## Input validation

Menu choices, numbers and weight ranges, phone (7 to 15 digits), email, real calendar dates (`YYYY-MM-DD`, with leap years), duplicate IDs, missing records, and blocked deletions of customers or parcels that are still active.
